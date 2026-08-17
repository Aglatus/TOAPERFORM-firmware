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
