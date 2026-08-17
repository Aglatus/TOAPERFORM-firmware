#pragma once
// =====================================================
// TOAPERFORM - PlayerMath.h
// SAF HESAPLAMA MODULU: hicbir Arduino/donanim bagimliligi yoktur (Serial,
// WiFi, LittleFS, GPIO YOK). Sadece standart C++ kullanir, bu yuzden hem
// ESP32'de hem PC'de (host derleyicisiyle) derlenip test edilebilir.
// 2026-08: kapsam SADECE nabiz tabanli yorgunluk skoruna daraltildi - GPS
// (kisisel sprint/tempo esikleri, metabolik guc), ACWR/Monotonluk ve
// Oyuncu Karti/Yetenek/Gelisim puanlamasi kullanici talebiyle kaldirildi.
// =====================================================
#include "Config.h"

namespace PlayerMath {

// ---------------- Yorgunluk / Risk ----------------
struct FatigueInputs {
  // Nabiz tabanli ic yuk (bkz. Config.h "Nabiz tabanli Ic Yuk" notu): caller
  // (TOAPERFORM.ino) her saniye, GUVENILIR bir nabiz sinyali varken (bkz.
  // HeartRateHardware::hasFreshSignal) su hesabi biriktirir:
  //   pctOfPersonalMax = (o anki bpm / sezon rekoru bpm) * 100
  //   excess = max(0, pctOfPersonalMax - HR_FATIGUE_BASELINE_PCT)
  //   hrLoadAccum += excess
  // Sezon rekoru nabiz henuz PERSONAL_MIN_HR_SAMPLE'a ulasmadiysa (yetersiz
  // veri) bu deger 0 birakilmali - kisisellestirme icin anlamli bir tavan yok
  // demektir.
  float hrLoadAccum = 0;
};

struct RiskResult {
  int score = 0;
  const char* status = "NORMAL";   // "NORMAL" | "UYARI" | "KRITIK"
  const char* colorHex = "#22c55e";
};

// Yorgunluk skoru SADECE nabiz tabanli ic yukten (hrLoadAccum) hesaplanir.
RiskResult calculateFatigueRisk(const FatigueInputs& in);

// Son FATIGUE_TREND_WINDOW saniyedeki yorgunluk artis hizina bakarak KRITIK
// esigine (40) kac dakikada ulasilacagini dogrusal olarak tahmin eder.
// Saniyede bir addSample() ile beslenmelidir.
class FatigueTrendTracker {
public:
  void reset();
  void addSample(int fatigueScore);
  float trendPerMin() const { return trendPerMin_; }
  int etaMinutes() const { return etaMinutes_; }  // -1 = gecerli tahmin yok

private:
  int buffer_[FATIGUE_TREND_WINDOW] = {};
  int index_ = 0;
  int count_ = 0;
  float trendPerMin_ = 0;
  int etaMinutes_ = -1;
};

// ---------------- Nabiz Bolgeleri ----------------
// pctOfPersonalMax: (o anki bpm / sezon rekoru bpm) * 100. Donus: 0 = Z1
// esiginin altinda (dinlenme/isinma sayilir, hicbir bolgeye eklenmez), 1-5 =
// Z1..Z5 (bkz. Config.h HR_ZONEn_MIN_PCT).
int hrZoneFromPercent(float pctOfPersonalMax);

}  // namespace PlayerMath
