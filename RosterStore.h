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
#include "PlayerMath.h"

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

  // ---------------- ACWR / Monotonluk (bkz. Config.h "ACWR" notu) ----------------
  // Bir antrenman sonunda o oyuncunun GUNLUK toplam yukune (dayIndex = epoch
  // saniye/86400) bu seansin yukunu ekler - ayni gun icinde birden fazla
  // antrenman olursa toplanir. LOADS_FILE'a (tum oyuncular icin ORTAK dosya,
  // playerId ile ayirt edilir) eklenir; ACWR_MAX_LOOKBACK_DAYS'ten eski
  // kayitlar periyodik olarak budanir.
  void recordDailyLoad(int id, long dayIndex, float load);

  // O oyuncunun LOADS_FILE'daki (en fazla MAX_LOAD_ENTRIES_PER_PLAYER kadar,
  // en yeniler tutulur) gunluk yuklerinden ACWR/monotonluk hesaplar.
  PlayerMath::AcwrResult acwrForPlayer(int id, long todayIdx) const;

  // ---------------- Gunluk Wellness Anketi (bkz. Config.h notu) ----------------
  // Ayni gun icin tekrar gonderilirse UZERINE YAZILMAZ (dosyaya yeni satir
  // eklenir, okurken O GUNUN EN SON kaydi kullanilir) - boylece bir oyuncu
  // anketi yanlislikla iki kez doldurursa ikincisi gecerli olur.
  void recordWellness(int id, long dayIndex, int sleep, int fatigue, int soreness, int stress, int mood);

  struct WellnessEntry {
    bool hasData = false;
    int sleep = 0, fatigue = 0, soreness = 0, stress = 0, mood = 0;
    int sum = 0;                          // 5-50, dusuk = iyi
    const char* band = "Veri yok";
  };
  // O oyuncunun dayIndex gunu icin (varsa) en son kaydedilen anketi doner.
  WellnessEntry todayWellness(int id, long dayIndex) const;

private:
  RosterPlayer players_[MAX_ROSTER_PLAYERS];
  int count_ = 0;
  int nextId_ = 1;

  void load();
  void saveAll();
};
