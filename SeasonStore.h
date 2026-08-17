#pragma once
// =====================================================
// TOAPERFORM - SeasonStore.h
// Donanim adaptoru: LittleFS uzerinde sezon pasaportu (season.dat) ve
// antrenman gecmisi (history.jsonl) kalictigini yonetir. Dosya bicimleri ve
// yorumlar orijinal .ino'dan aynen tasindi.
// 2026-08: kapsam SADECE nabiza daraltildi - GPS'e dayali alanlar
// (maxSpeedEver/totalDistance/totalSprints), sicrama ve mevki kaldirildi.
// =====================================================
#include <LittleFS.h>
#include "Config.h"

struct SeasonPassport {
  int sessionCount = 0;
  float totalLoad = 0;
  float maxHrEver = 0;     // bpm - nabiz tabanli ic yuk hesabinin kisisel tavani (bkz. Config.h)
};

// Dosyadan bir satiri sabit tampona okur (String heap tahsisi olmadan).
// Basari durumunda true, dosya sonuna gelindiyse false doner.
bool readLineToBuffer(File& f, char* buf, size_t bufSize);

class SeasonStore {
public:
  void begin();  // LittleFS.begin + kayitlari yukler

  const SeasonPassport& passport() const { return passport_; }
  int historySessionCounter() const { return historySessionCounter_; }

  // sessionTs: reset anindaki epoch saniye (telefon tarayicisindan gelir,
  // cihazda RTC yok). rpe: 1-10 (0 = girilmedi -> Foster session-RPE yerine
  // fatigueScore tabanli tahmini yuk kullanilir, "est":true isaretlenir).
  // MIN_HISTORY_SESSION_SECONDS altindaki seanslar sessizce atlanir.
  void saveSessionToHistory(unsigned long sessionTs, int rpe, unsigned long sessionSeconds,
    int fatigueScoreForEstimatedLoad, float sessionMaxHr);

  // Yeni bir sporcu cihazi devraldiginda kariyer/sezon rekorlarini sifirlar.
  // Antrenman gecmisine (history.jsonl) dokunmaz.
  void resetSeasonPassport();

  // Gecmisten TEK bir antrenmani ("n" alanina gore) siler. trimHistoryIfNeeded
  // gibi (bkz. .cpp) dosyayi tamamen yeniden yazar - passport toplamlarina
  // (sessionCount/totalLoad/maxHrEver) GERI ALMA yapilmaz, bunlar kalici
  // "ever" tipi kayitlardir (eski antrenmanlarin otomatik budanmasinda da
  // ayni sekilde davranilir).
  void deleteHistoryEntry(int n);

  // /history, /backup handler'larinin akis halinde okumasi icin dogrudan
  // dosya erisimi.
  File openHistoryFile() const;

private:
  SeasonPassport passport_;
  int historySessionCounter_ = 0;

  void loadSeasonPassport();
  void savePassport();
  void loadHistoryCounter();
  void trimHistoryIfNeeded();
};
