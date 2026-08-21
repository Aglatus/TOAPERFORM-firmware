#include "WebRoutes.h"
#include "Globals.h"
#include "WebUi.h"
#include "PlayerMath.h"
#include <LittleFS.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void sendNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
}

// =====================================================
// / ve /data
// =====================================================
static void handleRoot() {
  // SAHA BULGUSU (2026-08): bu route hic no-cache basligi gondermiyordu -
  // telefon tarayicisi ana sayfayi (tum CSS/JS'iyle) onbellekten gosterip
  // firmware guncellemelerinin gorunmemesine yol aciyordu. Diger tum route'lar
  // zaten sendNoCacheHeaders() cagiriyordu, burasi unutulmustu.
  sendNoCacheHeaders();
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
  unsigned long now = millis();

  // Panel her /data cagrisinda telefonun GERCEK saatini (epoch saniye) yollar -
  // ESP32'nin RTC'si yok, ACWR'nin "bugun"u bu sekilde tahmin edilir (bkz.
  // Globals.h updateEpochSync/currentDayIndex).
  if (server.hasArg("ts")) {
    updateEpochSync(strtoul(server.arg("ts").c_str(), nullptr, 10));
  }
  long today = currentDayIndex();

  char timeBuf[16];
  formatTime(sessionSeconds, timeBuf, sizeof(timeBuf));

  // ~950 bayt/slot (isim+uyari+ongoru+ACWR+HRV+HRR+wellness+HRV taban+readiness+
  // solunum+ortostatik metinleri dahil en kotu durum) x HR_SLOTS + sarici icin
  // genis bir pay - bkz. SLOT_COUNT (Config.h/HeartRateHardware.h).
  static char json[HR_SLOTS * 1050 + 64];
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

    int rosterIdx = hasPlayer ? rosterStore.findIndexById(slotPlayerId[i]) : -1;

    float personalMaxHr = 0;
    if (rosterIdx >= 0) personalMaxHr = rosterStore.playerAt(rosterIdx).maxHrEver;
    bool baselineReady = hasPlayer && personalMaxHr >= PERSONAL_MIN_HR_SAMPLE;

    char trendWarning[100] = "";
    if (fatigueTrend[i].etaMinutes() >= 0) {
      snprintf(trendWarning, sizeof(trendWarning),
        "Bu tempoyla ~%d dakika icinde KRITIK seviyeye ulasilabilir", fatigueTrend[i].etaMinutes());
    }

    // ACWR: "bugun" bilinmiyorsa (henuz telefon senkron olmadiysa) ya da oyuncu
    // atanmamissa hesaplanmaz - varsayilan (yetersiz veri) bandiyla gonderilir.
    PlayerMath::AcwrResult acwr;
    if (hasPlayer && today >= 0) acwr = rosterStore.acwrForPlayer(slotPlayerId[i], today);

    // HRR testi: hem devam eden (hrrActive) hem tamamlanmis (son sonuc kalir,
    // yeni bir /reset veya yeni test baslayana kadar panelde gorunur) durumu
    // tasir. Dusus miktarlari (bkz. PlayerMath::calculateHrrDrop) sadece ilgili
    // dakika olculdukten sonra anlamli, aksi halde -1 (henuz yok).
    int hrr1 = PlayerMath::calculateHrrDrop(hrrHr0[i], hrrHr60[i]);
    int hrr2 = PlayerMath::calculateHrrDrop(hrrHr0[i], hrrHr120[i]);
    unsigned long hrrElapsedSec = hrrActive[i] ? (now - hrrStartMs[i]) / 1000UL : 0;

    // Ortostatik Toparlanma Testi: devam eden fazin gecen suresi (bkz.
    // Config.h/Globals.h ORTHO notu - iki fazli, sabit sureli, otomatik).
    unsigned long orthoElapsedSec = orthoActive[i] ? (now - orthoPhaseStartMs[i]) / 1000UL : 0;

    // Wellness anketi: bkz. Config.h "Gunluk Wellness Anketi" notu - panelden
    // antrenman oncesi elle doldurulur, nabizdan bagimsiz.
    RosterStore::WellnessEntry wellness;
    if (hasPlayer && today >= 0) wellness = rosterStore.todayWellness(slotPlayerId[i], today);

    // HRV Taban Cizgisi + Composite Hazir Olma Skoru (2026-08 eklemeleri, bkz.
    // RosterStore.h/PlayerMath.h notlari) - ikisi de TAMAMEN otomatik/gercek
    // veriden turer, bilerek fatigueScore/canli nabiz KATILMAZ (bkz.
    // PlayerMath::ReadinessInputs notu - readiness antrenman ONCESI durumu
    // ozetler, "su an" metrikleri zaten ayri gosteriliyor).
    bool hrvBaselineReady = false;
    float hrvBaselineRmssd = 0;
    float hrvDeviationPct = 0;
    PlayerMath::ReadinessInputs readinessIn;
    readinessIn.hasWellness = wellness.hasData;
    readinessIn.wellnessSum = wellness.sum;
    readinessIn.hasAcwr = (acwr.daysWithData >= 3);
    readinessIn.acwr = acwr.acwr;
    readinessIn.acwrDays = acwr.daysWithData;
    if (rosterIdx >= 0) {
      const RosterPlayer& rp = rosterStore.playerAt(rosterIdx);
      hrvBaselineReady = rp.hrvBaselineSessions >= HRV_BASELINE_MIN_SESSIONS;
      hrvBaselineRmssd = rp.hrvBaselineRmssd;
      if (hrvBaselineReady && rp.hrvLastSessionRmssd > 0) {
        hrvDeviationPct = ((rp.hrvLastSessionRmssd - rp.hrvBaselineRmssd) / rp.hrvBaselineRmssd) * 100.0f;
        readinessIn.hasHrvBaseline = true;
        readinessIn.hrvDeviationPct = hrvDeviationPct;
      }
    }
    PlayerMath::ReadinessResult readiness = PlayerMath::calculateReadiness(readinessIn);

    off += snprintf(json + off, sizeof(json) - off,
      "%s{\"slot\":%d,\"bandLabel\":\"%s\",\"enabled\":%s,"
      "\"playerId\":%d,\"playerName\":\"%s\",\"baselineReady\":%s,"
      "\"bpm\":%d,\"connected\":%s,\"contact\":%s,\"fresh\":%s,\"status\":\"%s\","
      "\"pctMax\":%.0f,\"zone\":%d,\"zoneSec\":[%lu,%lu,%lu,%lu,%lu],"
      "\"fatigue\":%d,\"riskStatus\":\"%s\",\"riskColor\":\"%s\",\"warning\":\"%s\",\"trendWarning\":\"%s\","
      "\"acwr\":%.2f,\"acwrBand\":\"%s\",\"acwrDays\":%d,\"monotony\":%.2f,\"monotonyBand\":\"%s\","
      "\"rrSupported\":%s,\"rmssd\":%.1f,\"sdnn\":%.1f,\"pnn50\":%.1f,\"breathingRate\":%.1f,"
      "\"hrrActive\":%s,\"hrrElapsedSec\":%lu,\"hrrHr0\":%d,\"hrr1\":%d,\"hrr2\":%d,"
      "\"wellnessHasData\":%s,\"wellnessSum\":%d,\"wellnessBand\":\"%s\","
      "\"hrvBaselineReady\":%s,\"hrvBaselineRmssd\":%.1f,\"hrvDeviationPct\":%.1f,"
      "\"readinessReady\":%s,\"readinessScore\":%d,\"readinessBand\":\"%s\",\"readinessColor\":\"%s\","
      "\"orthoActive\":%s,\"orthoPhase\":%d,\"orthoElapsedSec\":%lu,"
      "\"orthoHr1\":%.0f,\"orthoHr2\":%.0f,\"orthoRmssd1\":%.1f,\"orthoRmssd2\":%.1f}",
      i == 0 ? "" : ",",
      i, heartRateHardware.label(i), enabled ? "true" : "false",
      slotPlayerId[i], hasPlayer ? slotPlayerName[i] : "", baselineReady ? "true" : "false",
      heartRateBpm[i], heartRateHardware.isConnected(i) ? "true" : "false",
      heartRateContact[i] ? "true" : "false", heartRateSignalFresh[i] ? "true" : "false", hrStatus,
      hrPctOfMax[i], hrZoneNow[i],
      hrZoneSeconds[i][0], hrZoneSeconds[i][1], hrZoneSeconds[i][2], hrZoneSeconds[i][3], hrZoneSeconds[i][4],
      fatigueScore[i], riskStatus[i], riskColor[i], lastWarning[i], trendWarning,
      acwr.acwr, acwr.band, acwr.daysWithData, acwr.monotony, acwr.monotonyBand,
      hrRrSupported[i] ? "true" : "false", hrRmssdMs[i], hrSdnnMs[i], hrPnn50[i], breathingRateBpm[i],
      hrrActive[i] ? "true" : "false", hrrElapsedSec, hrrHr0[i], hrr1, hrr2,
      wellness.hasData ? "true" : "false", wellness.sum, wellness.band,
      hrvBaselineReady ? "true" : "false", hrvBaselineRmssd, hrvDeviationPct,
      readiness.hasEnoughData ? "true" : "false", readiness.score, readiness.band, readiness.colorHex,
      orthoActive[i] ? "true" : "false", orthoPhase[i], orthoElapsedSec,
      orthoHr1[i], orthoHr2[i], orthoRmssdResult1[i], orthoRmssdResult2[i]
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

// id=<roster id> - o oyuncunun son MAX_SESSION_LOG_ENTRIES_PER_PLAYER
// oturumunun ozetini (yorgunluk skoru + HRV taban sapmasi) doner, ESKIDEN
// YENIYE sirali (bkz. RosterStore::sessionLogForPlayer). Focus Modu acilinca
// AYRI bir fetch ile cekilir - /data gibi surekli yoklanmiyor.
static void handlePlayerTrend() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"id gerekli\"}");
    return;
  }
  int id = server.arg("id").toInt();
  if (rosterStore.findIndexById(id) < 0) {
    server.send(400, "application/json", "{\"error\":\"oyuncu bulunamadi\"}");
    return;
  }

  RosterStore::SessionLogEntry entries[MAX_SESSION_LOG_ENTRIES_PER_PLAYER];
  int n = rosterStore.sessionLogForPlayer(id, entries, MAX_SESSION_LOG_ENTRIES_PER_PLAYER);

  static char json[MAX_SESSION_LOG_ENTRIES_PER_PLAYER * 48 + 20];
  int off = 0;
  off += snprintf(json + off, sizeof(json) - off, "[");
  for (int i = 0; i < n; i++) {
    off += snprintf(json + off, sizeof(json) - off,
      "%s{\"dayIndex\":%ld,\"fatigue\":%d,\"hrvDeviationPct\":%.1f}",
      i == 0 ? "" : ",", entries[i].dayIndex, entries[i].fatigueScore, entries[i].hrvDeviationPct);
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
// Kalp Hizi Toparlanmasi (HRR) Testi
// =====================================================
// slot=<0..HR_SLOTS-1> - o slotun O ANKI guvenilir bpm'ini HR0 olarak
// yakalar (bkz. Config.h HRR notu - "efor bitti" ani manuel isaretlenir).
static void handleStartHrrTest() {
  if (!server.hasArg("slot")) {
    server.send(400, "application/json", "{\"error\":\"slot gerekli\"}");
    return;
  }
  int slot = server.arg("slot").toInt();
  if (slot < 0 || slot >= HR_SLOTS) {
    server.send(400, "application/json", "{\"error\":\"gecersiz slot\"}");
    return;
  }
  if (!heartRateSignalFresh[slot]) {
    server.send(400, "application/json", "{\"error\":\"guvenilir nabiz sinyali yok\"}");
    return;
  }

  startHrrTest(slot, heartRateBpm[slot]);

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"started\":true}");
}

// =====================================================
// Ortostatik Toparlanma Testi
// =====================================================
// slot=<0..HR_SLOTS-1> - Faz1'i (yatarken/otururken) baslatir, Faz2'ye
// (ayaktayken) ORTHO_PHASE_MS sonra otomatik gecer (bkz. Globals.h/
// TOAPERFORM.ino ORTHO notu - HRR'nin aksine "efor bitti" gibi bir ani
// algilamiyor, sadece zamanlayici kullanir).
static void handleStartOrthoTest() {
  if (!server.hasArg("slot")) {
    server.send(400, "application/json", "{\"error\":\"slot gerekli\"}");
    return;
  }
  int slot = server.arg("slot").toInt();
  if (slot < 0 || slot >= HR_SLOTS) {
    server.send(400, "application/json", "{\"error\":\"gecersiz slot\"}");
    return;
  }
  if (!heartRateSignalFresh[slot]) {
    server.send(400, "application/json", "{\"error\":\"guvenilir nabiz sinyali yok\"}");
    return;
  }

  startOrthoTest(slot);

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"started\":true}");
}

// =====================================================
// Gunluk Wellness Anketi
// =====================================================
// id=<roster id>&sleep=&fatigue=&soreness=&stress=&mood= (her biri 1-10,
// bkz. Config.h "Gunluk Wellness Anketi" notu). Bant/slota degil dogrudan
// oyuncuya baglidir - antrenmandan once, band takilmadan da doldurulabilir.
static void handleWellness() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"id gerekli\"}");
    return;
  }
  int id = server.arg("id").toInt();
  if (rosterStore.findIndexById(id) < 0) {
    server.send(400, "application/json", "{\"error\":\"oyuncu bulunamadi\"}");
    return;
  }

  long today = currentDayIndex();
  if (today < 0) {
    server.send(400, "application/json", "{\"error\":\"cihaz henuz saat senkronu almadi, paneli acik tutup tekrar deneyin\"}");
    return;
  }

  auto clamp1to10 = [](int v) { return v < 1 ? 1 : (v > 10 ? 10 : v); };
  int sleep = clamp1to10(server.hasArg("sleep") ? server.arg("sleep").toInt() : 5);
  int fatigue = clamp1to10(server.hasArg("fatigue") ? server.arg("fatigue").toInt() : 5);
  int soreness = clamp1to10(server.hasArg("soreness") ? server.arg("soreness").toInt() : 5);
  int stress = clamp1to10(server.hasArg("stress") ? server.arg("stress").toInt() : 5);
  int mood = clamp1to10(server.hasArg("mood") ? server.arg("mood").toInt() : 5);

  rosterStore.recordWellness(id, today, sleep, fatigue, soreness, stress, mood);

  sendNoCacheHeaders();
  server.send(200, "application/json", "{\"saved\":true}");
}

// =====================================================
// Reset
// =====================================================
static void handleReset() {
  unsigned long sessionTs = 0;
  if (server.hasArg("ts")) {
    sessionTs = strtoul(server.arg("ts").c_str(), nullptr, 10);
  }
  updateEpochSync(sessionTs);

  int rpe = 0;
  if (server.hasArg("rpe")) {
    rpe = server.arg("rpe").toInt();
    if (rpe < 0 || rpe > 10) rpe = 0;
  }

  // Mac Gunu vs Antrenman ayrimi (bkz. Config.h MATCH_LOAD_MULTIPLIER notu) -
  // sadece ACWR'ye giren GUNLUK yuku carpar, "type" gonderilmezse (eski
  // istemciler/varsayilan) antrenman sayilir, hicbir sey degismez.
  bool isMatch = server.hasArg("type") && server.arg("type") == "match";
  float loadMultiplier = isMatch ? MATCH_LOAD_MULTIPLIER : 1.0f;

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
      // NOT: kisisel rekor/toplam yuk (updateAfterSession, "omur boyu"
      // istatistik) CARPILMAMIS ham yukle guncellenir - mac agirligi SADECE
      // ACWR'nin GUNLUK yukune uygulanir, kariyer toplamini sismesin diye.
      rosterStore.updateAfterSession(slotPlayerId[i], sessionLoad, sessionMaxHr[i]);

      // HRV Taban Cizgisi (2026-08 ekleme): bu oturumda gecerli RR-interval
      // verisi toplandiysa (hrRmssdSessionCount>0) ortalamasi kisisel tabana
      // katilir - bkz. RosterStore::updateHrvBaseline/PlayerMath::updateHrvBaselineEwma.
      if (hrRmssdSessionCount[i] > 0) {
        float sessionAvgRmssd = hrRmssdSessionSum[i] / hrRmssdSessionCount[i];
        rosterStore.updateHrvBaseline(slotPlayerId[i], sessionAvgRmssd);
      }

      // ACWR icin: bu antrenmanin/macin yukunu o oyuncunun GUNLUK toplamina
      // ekler (bkz. Config.h ACWR notu). sessionTs yoksa (telefon saatini hic
      // gondermediyse) gun-indeksi bilinmez, atlanir.
      if (sessionTs > 0) {
        rosterStore.recordDailyLoad(slotPlayerId[i], (long)(sessionTs / 86400UL), sessionLoad * loadMultiplier);

        // Oyuncu Bazli Cok-Oturumlu Trend (2026-08 ekleme): bu oturumun ozetini
        // (yorgunluk skoru + varsa HRV taban sapmasi) SESSION_LOG_FILE'a ekler -
        // Focus Modu'ndaki trend grafigi icin (bkz. RosterStore::recordSessionLog).
        // HRV taban henuz hazir degilse (yeterli oturum birikmediyse) 0 yazilir -
        // /playertrend tuketicisi bunu "veri yok" olarak yorumlamali.
        float hrvDevForLog = 0;
        int rIdx = rosterStore.findIndexById(slotPlayerId[i]);
        if (rIdx >= 0) {
          const RosterPlayer& rp = rosterStore.playerAt(rIdx);
          if (rp.hrvBaselineSessions >= HRV_BASELINE_MIN_SESSIONS && rp.hrvBaselineRmssd > 0 && rp.hrvLastSessionRmssd > 0) {
            hrvDevForLog = ((rp.hrvLastSessionRmssd - rp.hrvBaselineRmssd) / rp.hrvBaselineRmssd) * 100.0f;
          }
        }
        rosterStore.recordSessionLog(slotPlayerId[i], (long)(sessionTs / 86400UL), fatigueScore[i], hrvDevForLog);
      }
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
// Kayit
// =====================================================
void registerWebRoutes() {
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/gpsconfig", handleGpsConfig);
  server.on("/roster", handleRosterList);
  server.on("/playertrend", handlePlayerTrend);
  server.on("/addplayer", HTTP_POST, handleAddPlayer);
  server.on("/assignslot", HTTP_POST, handleAssignSlot);
  server.on("/starthrrtest", HTTP_POST, handleStartHrrTest);
  server.on("/startorthotest", HTTP_POST, handleStartOrthoTest);
  server.on("/wellness", HTTP_POST, handleWellness);
  server.on("/reset", handleReset);
  server.on("/resetseason", handleResetSeason);
  server.on("/team", handleTeam);
  server.on("/history", handleHistory);
  server.on("/deletehistory", handleDeleteHistory);
  server.on("/backup", handleBackup);
  server.on("/report", handleReport);

  // Captive portal paths
  server.on("/generate_204", handleCaptive);
  server.on("/gen_204", handleCaptive);
  server.on("/hotspot-detect.html", handleRoot);
  server.on("/connecttest.txt", handleRoot);
  server.on("/ncsi.txt", handleRoot);
  server.onNotFound(handleRoot);
}
