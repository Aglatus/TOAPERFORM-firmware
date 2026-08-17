#include "WebRoutes.h"
#include "Globals.h"
#include "WebUi.h"
#include "PlayerMath.h"
#include <Update.h>
#include <LittleFS.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static bool otaAuthorized = false;

static void sendNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
}

// =====================================================
// / ve /data
// =====================================================
static void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// Panel bir kere acilista bunu okur (bkz. WebUi.h loadGpsConfig()) - "Sunucuya
// Gonder" butonunun hangi URL'e, hangi cihaz adiyla, hangi anahtarla POST
// atacagini soyler. /data gibi sik yoklanmiyor, tek seferlik.
static void handleGpsConfig() {
  static char json[300];
  snprintf(json, sizeof(json),
    "{\"deviceName\":\"%s\",\"uploadUrl\":\"%s\",\"deviceKey\":\"%s\"}",
    DEVICE_NAME, GPS_UPLOAD_URL, GPS_DEVICE_KEY);
  server.send(200, "application/json", json);
}

static void handleData() {
  char timeBuf[16];
  formatTime(sessionSeconds, timeBuf, sizeof(timeBuf));

  // ~450 bayt/slot (isim+uyari+ongoru metinleri dahil en kotu durum) x HR_SLOTS
  // + sarici icin genis bir pay - bkz. SLOT_COUNT (Config.h/HeartRateHardware.h).
  static char json[HR_SLOTS * 500 + 64];
  int off = 0;
  off += snprintf(json + off, sizeof(json) - off, "{\"trainingTime\":\"%s\",\"slots\":[", timeBuf);

  for (int i = 0; i < HR_SLOTS; i++) {
    bool enabled = heartRateHardware.slotEnabled(i);
    bool hasPlayer = (slotPlayerId[i] != 0);

    const char* hrStatus;
    if (!enabled) hrStatus = "Devre disi";
    else if (heartRateHardware.isConnected(i)) hrStatus = heartRateSignalFresh[i] ? "Bagli" : "Bagli (sinyal yok)";
    else if (heartRateHardware.isScanning()) hrStatus = "Araniyor";
    else hrStatus = "Bagli degil";

    float personalMaxHr = 0;
    if (hasPlayer) {
      int idx = rosterStore.findIndexById(slotPlayerId[i]);
      if (idx >= 0) personalMaxHr = rosterStore.playerAt(idx).maxHrEver;
    }
    bool baselineReady = hasPlayer && personalMaxHr >= PERSONAL_MIN_HR_SAMPLE;

    char trendWarning[100] = "";
    if (fatigueTrend[i].etaMinutes() >= 0) {
      snprintf(trendWarning, sizeof(trendWarning),
        "Bu tempoyla ~%d dakika icinde KRITIK seviyeye ulasilabilir", fatigueTrend[i].etaMinutes());
    }

    off += snprintf(json + off, sizeof(json) - off,
      "%s{\"slot\":%d,\"bandLabel\":\"%s\",\"enabled\":%s,"
      "\"playerId\":%d,\"playerName\":\"%s\",\"baselineReady\":%s,"
      "\"bpm\":%d,\"connected\":%s,\"contact\":%s,\"fresh\":%s,\"status\":\"%s\","
      "\"pctMax\":%.0f,\"zone\":%d,\"zoneSec\":[%lu,%lu,%lu,%lu,%lu],"
      "\"fatigue\":%d,\"riskStatus\":\"%s\",\"riskColor\":\"%s\",\"warning\":\"%s\",\"trendWarning\":\"%s\"}",
      i == 0 ? "" : ",",
      i, heartRateHardware.label(i), enabled ? "true" : "false",
      slotPlayerId[i], hasPlayer ? slotPlayerName[i] : "", baselineReady ? "true" : "false",
      heartRateBpm[i], heartRateHardware.isConnected(i) ? "true" : "false",
      heartRateContact[i] ? "true" : "false", heartRateSignalFresh[i] ? "true" : "false", hrStatus,
      hrPctOfMax[i], hrZoneNow[i],
      hrZoneSeconds[i][0], hrZoneSeconds[i][1], hrZoneSeconds[i][2], hrZoneSeconds[i][3], hrZoneSeconds[i][4],
      fatigueScore[i], riskStatus[i], riskColor[i], lastWarning[i], trendWarning
    );
  }

  off += snprintf(json + off, sizeof(json) - off, "]}");

  sendNoCacheHeaders();
  server.send(200, "application/json", json);
}

// =====================================================
// Oyuncu Roster'i (ortak havuz) + Bant Atamasi
// =====================================================
static void handleRosterList() {
  static char json[MAX_ROSTER_PLAYERS * 60 + 20];
  int off = 0;
  off += snprintf(json + off, sizeof(json) - off, "[");
  for (int i = 0; i < rosterStore.count(); i++) {
    const RosterPlayer& p = rosterStore.playerAt(i);
    off += snprintf(json + off, sizeof(json) - off,
      "%s{\"id\":%d,\"name\":\"%s\",\"maxHrEver\":%.0f}",
      i == 0 ? "" : ",", p.id, p.name, p.maxHrEver);
  }
  off += snprintf(json + off, sizeof(json) - off, "]");

  sendNoCacheHeaders();
  server.send(200, "application/json", json);
}

static void handleAddPlayer() {
  if (!server.hasArg("name") || server.arg("name").length() == 0) {
    server.send(400, "application/json", "{\"error\":\"isim gerekli\"}");
    return;
  }

  int id = rosterStore.addPlayer(server.arg("name").c_str());
  if (id < 0) {
    server.send(400, "application/json", "{\"error\":\"oyuncu listesi dolu\"}");
    return;
  }

  char json[48];
  snprintf(json, sizeof(json), "{\"id\":%d}", id);
  sendNoCacheHeaders();
  server.send(200, "application/json", json);
}

// slot=<0..HR_SLOTS-1>&id=<roster id> - id=0 (veya yok) slotu bosaltir.
static void handleAssignSlot() {
  if (!server.hasArg("slot")) {
    server.send(400, "application/json", "{\"error\":\"slot gerekli\"}");
    return;
  }
  int slot = server.arg("slot").toInt();
  if (slot < 0 || slot >= HR_SLOTS) {
    server.send(400, "application/json", "{\"error\":\"gecersiz slot\"}");
    return;
  }

  int id = server.hasArg("id") ? server.arg("id").toInt() : 0;

  if (id == 0) {
    slotPlayerId[slot] = 0;
    slotPlayerName[slot][0] = '\0';
  } else {
    int idx = rosterStore.findIndexById(id);
    if (idx < 0) {
      server.send(400, "application/json", "{\"error\":\"oyuncu bulunamadi\"}");
      return;
    }
    slotPlayerId[slot] = id;
    strncpy(slotPlayerName[slot], rosterStore.playerAt(idx).name, sizeof(slotPlayerName[slot]) - 1);
    slotPlayerName[slot][sizeof(slotPlayerName[slot]) - 1] = '\0';
  }

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"assigned\":true}");
}

// =====================================================
// Reset
// =====================================================
static void handleReset() {
  unsigned long sessionTs = 0;
  if (server.hasArg("ts")) {
    sessionTs = strtoul(server.arg("ts").c_str(), nullptr, 10);
  }

  int rpe = 0;
  if (server.hasArg("rpe")) {
    rpe = server.arg("rpe").toInt();
    if (rpe < 0 || rpe > 10) rpe = 0;
  }

  // Klasik (tek) antrenman gecmisi - hep "Bant 1"/slot 0 verisiyle, roster'dan
  // BAGIMSIZ (bkz. SeasonStore.h / RosterStore.h yorumlari - iki ayri ozellik).
  seasonStore.saveSessionToHistory(sessionTs, rpe, sessionSeconds, fatigueScore[0], sessionMaxHr[0]);

  // Roster: atanmis HER oyuncunun kisisel rekoru/toplam yuku guncellenir - ayni
  // Foster session-RPE formulu (dakika x RPE, RPE girilmediyse o slotun kendi
  // fatigueScore'undan tahmini yuk), bkz. SeasonStore.cpp yorumu.
  if (sessionSeconds >= MIN_HISTORY_SESSION_SECONDS) {
    float sessionMinutes = sessionSeconds / 60.0f;
    bool loadIsEstimated = (rpe <= 0);
    for (int i = 0; i < HR_SLOTS; i++) {
      if (slotPlayerId[i] == 0) continue;
      float sessionLoad = loadIsEstimated
        ? (sessionMinutes * (fatigueScore[i] / 10.0f))
        : (sessionMinutes * rpe);
      rosterStore.updateAfterSession(slotPlayerId[i], sessionLoad, sessionMaxHr[i]);
    }
  }

  resetSession();

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"reset\":true}");
}

static void handleDeleteHistory() {
  if (server.hasArg("n")) {
    seasonStore.deleteHistoryEntry(server.arg("n").toInt());
  }

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"deleted\":true}");
}

static void handleResetSeason() {
  seasonStore.resetSeasonPassport();

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"seasonReset\":true}");
}

// =====================================================
// Takim
// =====================================================
static void handleTeam() {
  // Bu cihazdan HR_SLOTS kadar "self" satiri + hub'in gordugu MAX_PEERS kadar
  // uzak cihaz satiri olabilir - her biri ~130 bayt (isim/risk/sure dahil).
  static char json[(HR_SLOTS + MAX_PEERS) * 130 + 64];
  size_t offset = 0;
  offset += snprintf(json + offset, sizeof(json) - offset,
    "{\"isHub\":%s,\"players\":[", DEVICE_IS_HUB ? "true" : "false");

  bool first = true;

  if (DEVICE_IS_HUB) {
    unsigned long now = millis();
    teamNetwork.expireStalePeers(now);

    char selfDuration[16];
    formatTime(sessionSeconds, selfDuration, sizeof(selfDuration));

    // Bu cihazda atanmis HER oyuncu icin ayri bir "self" satiri (bkz.
    // TOAPERFORM.ino broadcast blogu - digerlerine de ayni sekilde slot
    // basina yayin yapiliyor).
    for (int i = 0; i < HR_SLOTS; i++) {
      if (slotPlayerId[i] == 0) continue;
      offset += snprintf(json + offset, sizeof(json) - offset,
        "%s{\"name\":\"%s\",\"bpm\":%d,\"fatigue\":%d,\"risk\":\"%s\",\"zone\":%d,\"duration\":\"%s\",\"ago\":0}",
        first ? "" : ",",
        slotPlayerName[i], heartRateSignalFresh[i] ? heartRateBpm[i] : 0, fatigueScore[i], riskStatus[i],
        heartRateSignalFresh[i] ? hrZoneNow[i] : 0, selfDuration);
      first = false;
    }

    const PeerRecord* table = teamNetwork.peerTable();
    for (int i = 0; i < teamNetwork.maxPeers(); i++) {
      if (!table[i].used) continue;

      char durBuf[16];
      formatTime(table[i].data.sessionSeconds, durBuf, sizeof(durBuf));

      offset += snprintf(json + offset, sizeof(json) - offset,
        "%s{\"name\":\"%s\",\"bpm\":%d,\"fatigue\":%d,\"risk\":\"%s\",\"zone\":%d,\"duration\":\"%s\",\"ago\":%lu}",
        first ? "" : ",",
        table[i].data.deviceName, table[i].data.heartRateBpm, table[i].data.fatigueScore,
        table[i].data.riskStatus, table[i].data.hrZone, durBuf,
        (now - table[i].lastSeenMs) / 1000UL);
      first = false;
    }
  }

  offset += snprintf(json + offset, sizeof(json) - offset, "]}");

  sendNoCacheHeaders();
  server.send(200, "application/json", json);
}

// =====================================================
// Gecmis / Yedek
// =====================================================
static void handleHistory() {
  sendNoCacheHeaders();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  server.sendContent("[");
  bool first = true;

  File f = seasonStore.openHistoryFile();
  if (f) {
    char lineBuf[360];
    while (readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
      if (strlen(lineBuf) == 0) continue;
      if (!first) server.sendContent(",");
      server.sendContent(lineBuf);
      first = false;
    }
    f.close();
  }
  server.sendContent("]");
}

// Sezon pasaportu + tum antrenman gecmisini tek bir JSON dosyasi olarak disa
// aktarir - cihaz arizalanirsa ya da "Sezonu Sifirla" gibi geri alinamaz bir
// islem oncesinde veriyi telefona/PC'ye kaydetmek icin.
static void handleBackup() {
  const SeasonPassport& p = seasonStore.passport();

  sendNoCacheHeaders();
  server.sendHeader("Content-Disposition", "attachment; filename=toaperform_yedek.json");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  char header[220];
  snprintf(header, sizeof(header),
    "{\"deviceName\":\"%s\","
    "\"seasonSessionCount\":%d,\"seasonTotalLoad\":%.1f,\"seasonMaxHrEver\":%.0f,\"history\":[",
    DEVICE_NAME,
    p.sessionCount, p.totalLoad, p.maxHrEver
  );
  server.sendContent(header);

  bool first = true;
  File f = seasonStore.openHistoryFile();
  if (f) {
    char lineBuf[360];
    while (readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
      if (strlen(lineBuf) == 0) continue;
      if (!first) server.sendContent(",");
      server.sendContent(lineBuf);
      first = false;
    }
    f.close();
  }
  server.sendContent("]}");
}

static void handleCaptive() {
  server.sendHeader("Location", "http://192.168.4.1", true);
  server.send(302, "text/plain", "");
}

static void handleReport() {
  server.send_P(200, "text/html", REPORT_HTML);
}

// =====================================================
// OTA
// =====================================================
static void handleUpdatePage() {
  if (!server.authenticate(OTA_USER, OTA_PASSWORD)) {
    return server.requestAuthentication();
  }
  server.send_P(200, "text/html", OTA_HTML);
}

static void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    otaAuthorized = server.authenticate(OTA_USER, OTA_PASSWORD);

    if (!otaAuthorized) {
      Serial.println("OTA reddedildi: yetkisiz erisim");
      return;
    }

    Serial.println("OTA basladi");

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaAuthorized) return;

    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!otaAuthorized) return;

    if (Update.end(true)) {
      Serial.println("OTA tamamlandi");
    } else {
      Update.printError(Serial);
    }
  }
}

static void handleUpdateComplete() {
  if (!server.authenticate(OTA_USER, OTA_PASSWORD)) {
    return server.requestAuthentication();
  }

  server.sendHeader("Connection", "close");
  server.send(200, "text/plain",
    (otaAuthorized && !Update.hasError()) ? "Guncelleme basarili. Cihaz yeniden baslatiliyor..." : "Guncelleme basarisiz!");
  delay(1000);

  if (otaAuthorized && !Update.hasError()) {
    ESP.restart();
  }

  otaAuthorized = false;
}

// =====================================================
// Kayit
// =====================================================
void registerWebRoutes() {
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/gpsconfig", handleGpsConfig);
  server.on("/roster", handleRosterList);
  server.on("/addplayer", HTTP_POST, handleAddPlayer);
  server.on("/assignslot", HTTP_POST, handleAssignSlot);
  server.on("/reset", handleReset);
  server.on("/resetseason", handleResetSeason);
  server.on("/team", handleTeam);
  server.on("/history", handleHistory);
  server.on("/deletehistory", handleDeleteHistory);
  server.on("/backup", handleBackup);
  server.on("/report", handleReport);
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, handleUpdateComplete, handleUpdateUpload);

  // Captive portal paths
  server.on("/generate_204", handleCaptive);
  server.on("/gen_204", handleCaptive);
  server.on("/hotspot-detect.html", handleRoot);
  server.on("/connecttest.txt", handleRoot);
  server.on("/ncsi.txt", handleRoot);
  server.onNotFound(handleRoot);
}
