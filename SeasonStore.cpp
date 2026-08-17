#include "SeasonStore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool readLineToBuffer(File& f, char* buf, size_t bufSize) {
  size_t len = 0;
  bool gotChar = false;

  while (f.available()) {
    int c = f.read();
    if (c < 0) break;

    gotChar = true;
    if (c == '\n') break;
    if (c == '\r') continue;

    if (len < bufSize - 1) {
      buf[len++] = (char)c;
    }
  }
  buf[len] = '\0';
  return gotChar;
}

void SeasonStore::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS baslatilamadi. Gecmis antrenman kaydi devre disi.");
    return;
  }
  loadHistoryCounter();
  loadSeasonPassport();
}

void SeasonStore::loadHistoryCounter() {
  historySessionCounter_ = 0;

  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) return;

  char lineBuf[360];
  while (readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) > 0) historySessionCounter_++;
  }
  f.close();
}

void SeasonStore::trimHistoryIfNeeded() {
  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) return;

  int lineCount = 0;
  char lineBuf[360];
  while (readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) > 0) lineCount++;
  }
  f.close();

  if (lineCount <= MAX_HISTORY_ENTRIES) return;

  int skip = lineCount - MAX_HISTORY_ENTRIES;

  File src = LittleFS.open(HISTORY_FILE, "r");
  File tmp = LittleFS.open("/history.tmp", "w");
  if (!src || !tmp) {
    if (src) src.close();
    if (tmp) tmp.close();
    return;
  }

  int idx = 0;
  while (readLineToBuffer(src, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) == 0) continue;

    idx++;
    if (idx > skip) {
      tmp.println(lineBuf);
    }
  }
  src.close();
  tmp.close();

  LittleFS.remove(HISTORY_FILE);
  LittleFS.rename("/history.tmp", HISTORY_FILE);
}

void SeasonStore::loadSeasonPassport() {
  File f = LittleFS.open(SEASON_FILE, "r");
  if (!f) return;

  char buf[120];
  if (readLineToBuffer(f, buf, sizeof(buf))) {
    sscanf(buf, "%d,%f,%f", &passport_.sessionCount, &passport_.totalLoad, &passport_.maxHrEver);
  }
  f.close();
}

void SeasonStore::savePassport() {
  char buf[120];
  snprintf(buf, sizeof(buf), "%d,%.1f,%.0f",
    passport_.sessionCount, passport_.totalLoad, passport_.maxHrEver);

  File f = LittleFS.open(SEASON_FILE, "w");
  if (f) {
    f.println(buf);
    f.close();
  }
}

void SeasonStore::resetSeasonPassport() {
  passport_ = SeasonPassport();
  savePassport();
}

// sessionTs: reset anindaki epoch saniye (telefon tarayicisindan gelir, cihazda RTC yok).
// rpe: 1-10 arasi subjektif zorluk algisi (0 = girilmedi).
// load: Foster session-RPE yontemi (dakika x RPE, "AU" birimi). Kaynak: Foster C. ve ark.,
// "A new approach to monitoring exercise training", J Strength Cond Res, 2001.
// Antrenman yuku: RPE girildiyse Foster'in session-RPE yontemi (dakika x RPE)
// kullanilir - "internal load" (subjektif). RPE girilmediyse yuk SIFIR
// SAYILMAZ - onun yerine zaten hesaplanan fatigueScore (nabiz tabanli ic yuk)
// 0-10 olcegine indirgenip AYNI formulle (dakika x yogunluk) kullanilir.
// Seffaflik icin bu tahmini yuk "est":true olarak isaretlenir - sporcunun
// bildirdigi gercek RPE ile ASLA kariştirilmaz.
void SeasonStore::saveSessionToHistory(unsigned long sessionTs, int rpe, unsigned long sessionSeconds,
  int fatigueScoreForEstimatedLoad, float sessionMaxHr) {
  if (sessionSeconds < MIN_HISTORY_SESSION_SECONDS) return;

  historySessionCounter_++;

  float sessionMinutes = sessionSeconds / 60.0f;
  bool loadIsEstimated = (rpe <= 0);
  float sessionLoad = loadIsEstimated ? (sessionMinutes * (fatigueScoreForEstimatedLoad / 10.0f)) : (sessionMinutes * rpe);

  unsigned long h = sessionSeconds / 3600;
  unsigned long m = (sessionSeconds % 3600) / 60;
  unsigned long s = sessionSeconds % 60;
  char durationBuf[16];
  snprintf(durationBuf, sizeof(durationBuf), "%02lu:%02lu:%02lu", h, m, s);

  char line[220];
  snprintf(line, sizeof(line),
    "{\"n\":%d,\"ts\":%lu,\"duration\":\"%s\",\"maxHr\":%.0f,\"rpe\":%d,\"load\":%.1f,\"est\":%s}",
    historySessionCounter_,
    sessionTs,
    durationBuf,
    sessionMaxHr,
    rpe,
    sessionLoad,
    loadIsEstimated ? "true" : "false"
  );

  File f = LittleFS.open(HISTORY_FILE, "a");
  if (f) {
    f.println(line);
    f.close();
  }

  trimHistoryIfNeeded();

  passport_.sessionCount++;
  passport_.totalLoad += sessionLoad;
  if (sessionMaxHr > passport_.maxHrEver) passport_.maxHrEver = sessionMaxHr;
  savePassport();
}

File SeasonStore::openHistoryFile() const {
  return LittleFS.open(HISTORY_FILE, "r");
}

void SeasonStore::deleteHistoryEntry(int n) {
  File src = LittleFS.open(HISTORY_FILE, "r");
  if (!src) return;

  File tmp = LittleFS.open("/history.tmp", "w");
  if (!tmp) {
    src.close();
    return;
  }

  char lineBuf[360];
  char needle[24];
  snprintf(needle, sizeof(needle), "\"n\":%d,", n);

  while (readLineToBuffer(src, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) == 0) continue;
    if (strstr(lineBuf, needle) != nullptr) continue;  // bu satiri atla (silinen kayit)
    tmp.println(lineBuf);
  }
  src.close();
  tmp.close();

  LittleFS.remove(HISTORY_FILE);
  LittleFS.rename("/history.tmp", HISTORY_FILE);
}
