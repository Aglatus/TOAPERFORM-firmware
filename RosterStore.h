#pragma once
// =====================================================
// TOAPERFORM - RosterStore.h
// Donanim adaptoru: LittleFS uzerinde ORTAK OYUNCU LISTESINI (roster.dat)
// kalicilastirir. SeasonStore'daki tek/kalici pasaporttan farki: burada N
// oyuncu (bkz. Config.h MAX_ROSTER_PLAYERS) ayni cihazda kayitli tutulur,
// cunku bantlar (Polar saatler) ortak havuzdan dagitilir - bir oyuncunun
// kisisel nabiz rekoru hangi fiziksel bandi taktigina degil, panelden
// sectigi roster kaydina bagli olmali (bkz. Config.h "Oyuncu Roster'i" notu).
//
// SeasonStore (season.dat/history.jsonl) BILEREK degistirilmedi - o hala
// cihazin kendi "Bant 1" (slot 0) antrenman gecmisini (RPE gibi) tutan, ayri
// ve daha eski bir ozellik. Roster sadece CANLI %HRmax/bolge hesabi icin
// gereken kisisel nabiz tavanini (maxHrEver) tutar.
// =====================================================
#include <LittleFS.h>
#include "Config.h"

struct RosterPlayer {
  int id = 0;
  char name[ROSTER_NAME_MAX] = "";
  int sessionCount = 0;
  float totalLoad = 0;
  float maxHrEver = 0;   // bpm - bu oyuncunun kisisel nabiz tavani (bkz. Config.h HR_ZONEn_MIN_PCT)
};

class RosterStore {
public:
  void begin();  // LittleFS'ten roster.dat'i yukler (SeasonStore.begin() zaten LittleFS.begin yaptiysa tekrar zararsizdir)

  int count() const { return count_; }
  const RosterPlayer& playerAt(int idx) const { return players_[idx]; }

  // id'ye gore roster_ icindeki index'i bulur, yoksa -1.
  int findIndexById(int id) const;

  // Yeni oyuncu ekler (isim sanitize edilir: virgul/yeni satir/tirnak
  // temizlenir, ROSTER_NAME_MAX-1 karaktere kirpilir). Basarili olursa yeni
  // id'yi, roster doluysa -1 doner.
  int addPlayer(const char* rawName);

  // Bir antrenman sonunda (o slota atanmis oyuncunun) rekorlarini gunceller
  // ve dosyaya yazar. sessionMaxHr rekor kirdiysa maxHrEver guncellenir.
  void updateAfterSession(int id, float sessionLoad, float sessionMaxHr);

private:
  RosterPlayer players_[MAX_ROSTER_PLAYERS];
  int count_ = 0;
  int nextId_ = 1;

  void load();
  void saveAll();
};
