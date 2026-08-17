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
#include <stdint.h>
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

// ---------------- ACWR (Akut:Kronik Is Yuku Orani) - EWMA ----------------
// Kaynak: Williams S ve ark., Br J Sports Med 2016; Murray NB, Gabbett TJ ve
// ark., Br J Sports Med 2017. Antrenman Monotonlugu/Zorlanmasi: Foster C ve
// ark., J Strength Cond Res 2001.
struct DayLoad {
  long dayIndex = 0;
  float load = 0;
};

struct AcwrResult {
  float ewmaAcute = 0;
  float ewmaChronic = 0;
  float acwr = 0;
  int daysWithData = 0;
  const char* band = "Yetersiz veri";
  float monotony = 0;
  float strain = 0;
  const char* monotonyBand = "Yetersiz veri";
};

// entries: gunluk toplam yuk kayitlari (siralamasi onemli degil, ayni gun
// icin birden fazla kayit olabilir - toplanir).
// todayIdx: bugunun gun-index'i (epoch saniye / 86400).
AcwrResult calculateAcwr(const DayLoad* entries, int entryCount, long todayIdx);

// ---------------- RMSSD (RR-interval tabanli, zaman-alani HRV metrigi) ----------------
// RMSSD = ardisik RR araliklari farklarinin kareler ortalamasinin karekoku.
// Standart, hesaplama olarak hafif (FFT gerektirmez) bir zaman-alani HRV
// metrigi. ONEMLI: bu fonksiyon SAF matematiktir, girdinin HAREKET HALINDE mi
// yoksa DINLENME HALINDE mi toplandigini bilmez/umursamaz - o ayrimi caller
// yapmali (bkz. Config.h HR_RR_BUFFER_SIZE / HeartRateHardware.h notlari:
// egzersiz sirasinda RMSSD "toparlanma HRV'si" ANLAMINA GELMEZ, sadece anlik
// bir gostergedir).
// rrMs: ardisik (zaman sirali) RR araliklari, milisaniye. count < 2 ise 0 doner.
float calculateRmssd(const uint16_t* rrMs, int count);

}  // namespace PlayerMath
