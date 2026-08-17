#include "RosterStore.h"
#include "SeasonStore.h"  // readLineToBuffer() - kucuk, genel amacli, tekrar yazmaya degmez
#include <stdio.h>
#include <string.h>

void RosterStore::begin() {
  load();
}

int RosterStore::findIndexById(int id) const {
  for (int i = 0; i < count_; i++) {
    if (players_[i].id == id) return i;
  }
  return -1;
}

static void sanitizeName(const char* rawName, char* out, size_t outSize) {
  size_t len = 0;
  for (const char* p = rawName; *p != '\0' && len < outSize - 1; p++) {
    char c = *p;
    if (c == ',' || c == '\n' || c == '\r' || c == '"') continue;  // dosya bicimini/JSON'u bozmasin
    out[len++] = c;
  }
  out[len] = '\0';
}

int RosterStore::addPlayer(const char* rawName) {
  if (count_ >= MAX_ROSTER_PLAYERS) return -1;

  RosterPlayer& p = players_[count_];
  p.id = nextId_++;
  sanitizeName(rawName, p.name, sizeof(p.name));
  if (p.name[0] == '\0') strcpy(p.name, "Oyuncu");
  p.sessionCount = 0;
  p.totalLoad = 0;
  p.maxHrEver = 0;
  count_++;

  saveAll();
  return p.id;
}

void RosterStore::updateAfterSession(int id, float sessionLoad, float sessionMaxHr) {
  int idx = findIndexById(id);
  if (idx < 0) return;

  players_[idx].sessionCount++;
  players_[idx].totalLoad += sessionLoad;
  if (sessionMaxHr > players_[idx].maxHrEver) players_[idx].maxHrEver = sessionMaxHr;

  saveAll();
}

// LOADS_FILE satir bicimi: playerId,dayIndex,load - TUM oyuncular icin ORTAK
// tek dosya (30 oyuncu x ~42 gun bile kucuk kalir, ayri dosya acmaya gerek yok).
void RosterStore::recordDailyLoad(int id, long dayIndex, float load) {
  File f = LittleFS.open(LOADS_FILE, "a");
  if (f) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%d,%ld,%.2f", id, dayIndex, load);
    f.println(buf);
    f.close();
  }

  // ACWR_MAX_LOOKBACK_DAYS'ten eski kayitlari buda - dosya sinirsiz buyumesin
  // (bkz. SeasonStore::trimHistoryIfNeeded ile ayni desen).
  File src = LittleFS.open(LOADS_FILE, "r");
  if (!src) return;

  File tmp = LittleFS.open("/loads.tmp", "w");
  if (!tmp) { src.close(); return; }

  char lineBuf[48];
  long cutoff = dayIndex - ACWR_MAX_LOOKBACK_DAYS;
  while (readLineToBuffer(src, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) == 0) continue;
    int lineId;
    long lineDay;
    float lineLoad;
    if (sscanf(lineBuf, "%d,%ld,%f", &lineId, &lineDay, &lineLoad) != 3) continue;
    if (lineDay < cutoff) continue;  // eski kaydi atla (buda)
    tmp.println(lineBuf);
  }
  src.close();
  tmp.close();

  LittleFS.remove(LOADS_FILE);
  LittleFS.rename("/loads.tmp", LOADS_FILE);
}

PlayerMath::AcwrResult RosterStore::acwrForPlayer(int id, long todayIdx) const {
  PlayerMath::DayLoad entries[MAX_LOAD_ENTRIES_PER_PLAYER];
  int entryCount = 0;

  File f = LittleFS.open(LOADS_FILE, "r");
  if (f) {
    char lineBuf[48];
    while (entryCount < MAX_LOAD_ENTRIES_PER_PLAYER && readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
      if (strlen(lineBuf) == 0) continue;
      int lineId;
      long lineDay;
      float lineLoad;
      if (sscanf(lineBuf, "%d,%ld,%f", &lineId, &lineDay, &lineLoad) != 3) continue;
      if (lineId != id) continue;
      entries[entryCount].dayIndex = lineDay;
      entries[entryCount].load = lineLoad;
      entryCount++;
    }
    f.close();
  }

  return PlayerMath::calculateAcwr(entries, entryCount, todayIdx);
}

void RosterStore::load() {
  count_ = 0;
  nextId_ = 1;

  File f = LittleFS.open(ROSTER_FILE, "r");
  if (!f) return;

  char lineBuf[96];
  while (readLineToBuffer(f, lineBuf, sizeof(lineBuf))) {
    if (strlen(lineBuf) == 0) continue;
    if (count_ >= MAX_ROSTER_PLAYERS) break;

    RosterPlayer p;
    char nameBuf[ROSTER_NAME_MAX] = "";
    // NOT: "%23" alan genisligi ROSTER_NAME_MAX-1'e (Config.h) sabit yazildi -
    // sscanf format string'i derleme zamaninda sabit olmali, ROSTER_NAME_MAX
    // degisirse burasi da elle guncellenmeli.
    int scanned = sscanf(lineBuf, "%d,%23[^,],%d,%f,%f",
      &p.id, nameBuf, &p.sessionCount, &p.totalLoad, &p.maxHrEver);
    if (scanned != 5) continue;

    strncpy(p.name, nameBuf, sizeof(p.name) - 1);
    p.name[sizeof(p.name) - 1] = '\0';

    players_[count_++] = p;
    if (p.id >= nextId_) nextId_ = p.id + 1;
  }
  f.close();
}

void RosterStore::saveAll() {
  File f = LittleFS.open(ROSTER_FILE, "w");
  if (!f) return;

  char buf[96];
  for (int i = 0; i < count_; i++) {
    const RosterPlayer& p = players_[i];
    snprintf(buf, sizeof(buf), "%d,%s,%d,%.1f,%.0f",
      p.id, p.name, p.sessionCount, p.totalLoad, p.maxHrEver);
    f.println(buf);
  }
  f.close();
}
