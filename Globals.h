#pragma once
// =====================================================
// TOAPERFORM - Globals.h
// Orkestrasyon katmani: alt sistem ornekleri (Season donanim adaptoru,
// AlertOutput, HeartRateHardware, RosterStore) ve bunlarin ARASINDAKI, tek
// bir modulun sahiplenemeyecegi oturum durumu. TOAPERFORM.ino (setup/loop)
// ve WebRoutes.cpp (HTTP handler'lari) bu paylasilan durumu buradan okur/yazar.
// 2026-08: kapsam SADECE nabiza daraltildi - GPS/hareket/saha kalibrasyonu
// state'i kaldirildi.
//
// 2026-08: COKLU OYUNCU - HeartRateHardware::SLOT_COUNT kadar BLE bandi ayni
// anda baglanabildigi icin (bkz. HeartRateHardware.h), asagidaki nabiz/
// yorgunluk/bolge durumu artik tek degil, SLOT BASINA bir dizi. Her slotun
// kimligi (hangi roster oyuncusu) slotPlayerId ile ayri tutulur - bantlar
// ortak havuzdan dagitildigi icin bir oyuncunun kisisel rekoru fiziksel banda
// degil, roster kaydina bagli olmali (bkz. Config.h, RosterStore.h).
// =====================================================
#include <WebServer.h>
#include <DNSServer.h>
#include "Config.h"
#include "SeasonStore.h"
#include "RosterStore.h"
#include "AlertOutput.h"
#include "PlayerMath.h"
#include "HeartRateHardware.h"
#include "TeamNetwork.h"

#define HR_SLOTS HeartRateHardware::SLOT_COUNT

// ---------------- Alt sistem ornekleri ----------------
extern SeasonStore seasonStore;    // bkz. asagidaki NOT - SLOT 0'in "klasik" antrenman gecmisi icin, roster'dan bagimsiz
extern RosterStore rosterStore;    // ortak oyuncu havuzu (isim + kisisel nabiz rekoru)
extern AlertOutput alertOutput;
extern PlayerMath::FatigueTrendTracker fatigueTrend[HR_SLOTS];
extern HeartRateHardware heartRateHardware;
extern TeamNetwork teamNetwork;

extern WebServer server;
extern DNSServer dnsServer;

// ---------------- Oturum durumu (cihaz genelinde, tum slotlar icin ortak saat) ----------------
extern unsigned long sessionStartMs;
extern unsigned long sessionSeconds;

// ---------------- Slot ataması (bant <-> roster oyuncusu) ----------------
// 0 = bu slota henuz roster'dan kimse atanmadi (RosterStore id'leri 1'den
// baslar, bkz. RosterStore.cpp nextId_ - bu yuzden 0 hicbir zaman gercek bir
// oyuncuya denk gelmez, ayri bir "atanmadi" bayragina gerek kalmaz). Atanana
// kadar o slotun BPM'i GORUNTULENIR ama yorgunluk/bolge hesabina KATILMAZ
// (kisisel rekor yoksa yuzde hesaplanamaz - bkz. PERSONAL_MIN_HR_SAMPLE mantigi).
extern int slotPlayerId[HR_SLOTS];
extern char slotPlayerName[HR_SLOTS][ROSTER_NAME_MAX];

// ---------------- Slot basina nabiz/yorgunluk/bolge durumu ----------------
// Baglanti/oturumdan bagimsiz nabiz okumalari (heartRateBpm/Contact/Fresh)
// resetSession() tarafindan sifirlanmaz - digerleri (fatigue/zone/sessionMaxHr)
// sifirlanir.
extern int heartRateBpm[HR_SLOTS];
extern bool heartRateContact[HR_SLOTS];
extern bool heartRateSignalFresh[HR_SLOTS];

extern float sessionMaxHr[HR_SLOTS];   // bu oturumda o slotun en yuksek GUVENILIR bpm'i
extern float hrLoadAccum[HR_SLOTS];    // nabiz tabanli ic yuk birikimi - bkz. PlayerMath.h FatigueInputs::hrLoadAccum
extern float hrPctOfMax[HR_SLOTS];     // anlik (bpm / o slotun kisisel rekoru) * 100 - guvenilir veri yoksa 0
extern int hrZoneNow[HR_SLOTS];        // anlik bolge, 0 (esik alti) veya 1-5 (Z1..Z5)
extern unsigned long hrZoneSeconds[HR_SLOTS][5];  // bu oturumda Z1..Z5'te gecirilen saniye (index 0=Z1..4=Z5)

extern int fatigueScore[HR_SLOTS];
extern char riskStatus[HR_SLOTS][16];
extern char riskColor[HR_SLOTS][9];
extern char lastWarning[HR_SLOTS][96];
extern unsigned long warningUntilMs[HR_SLOTS];

// RR-interval/HRV (bkz. Config.h HR_RR_BUFFER_SIZE, HeartRateHardware.h,
// PlayerMath::calculateHrv notlari). rrSupported[i]=false ise cihaz/mod bu
// veriyi hic gondermiyor demektir - panelde tamamen gizlenmeli.
extern float hrRmssdMs[HR_SLOTS];
extern float hrSdnnMs[HR_SLOTS];
extern float hrPnn50[HR_SLOTS];
extern bool hrRrSupported[HR_SLOTS];

// ---------------- HRV Taban Cizgisi - oturum-ici biriktirme (2026-08 ekleme) ----------------
// Oturum boyunca (rrSupported oldugu her 1Hz turda, gecerli bir RMSSD ornegi
// hesaplandiginda) hrRmssdMs ornekleri toplanir - oturum sonunda (WebRoutes
// handleReset) ortalamasi RosterStore::updateHrvBaseline'a verilir. Diger
// oturum-basi sayaclar gibi resetSession() tarafindan sifirlanir.
extern float hrRmssdSessionSum[HR_SLOTS];
extern int hrRmssdSessionCount[HR_SLOTS];

// ---------------- Solunum Hizi Tahmini (2026-08 ekleme, bkz. Config.h) ----------------
// Diger HRV alanlari gibi baglanti/oturumdan bagimsiz, salt donanimdan gelen
// bir deger - her 1Hz turda hesaplanir. 0 = gecerli tahmin yok (bkz.
// PlayerMath::estimateBreathingRate).
extern float breathingRateBpm[HR_SLOTS];

// ---------------- Kalp Hizi Toparlanmasi (HRR) testi (bkz. Config.h) ----------------
// Panelden manuel tetiklenir - "efor bitti" anini cihaz kendisi algilayamaz.
extern bool hrrActive[HR_SLOTS];
extern unsigned long hrrStartMs[HR_SLOTS];
extern int hrrHr0[HR_SLOTS];
extern int hrrHr60[HR_SLOTS];   // -1 = 1. dakika henuz olculmedi
extern int hrrHr120[HR_SLOTS];  // -1 = 2. dakika henuz olculmedi

// O anki bpm'i HR0 olarak yakalar, 1./2. dakika olcumlerini sifirlar ve
// sayaci baslatir. hasFreshSignal false ise (guvenilir bpm yoksa) sessizce
// hicbir sey yapmaz (caller - WebRoutes - bunu kontrol etmeli).
void startHrrTest(int slot, int currentBpm);

// ---------------- Ortostatik Toparlanma Testi (2026-08 ekleme, bkz. Config.h) ----------------
// Panelden manuel tetiklenir ("Testi Baslat") - Faz1 (yatarken/otururken,
// ORTHO_PHASE_MS sureyle) ve Faz2 (ayaktayken, ayni sure) SIRAYLA otomatik
// zamanlanir (HRR'nin aksine "efor bitti" gibi bir ani algilamak gerekmiyor,
// sadece sabit sureli iki pencere var). Panel faz gecisinde "AYAGA KALK"
// uyarisi gosterir - oyuncu/koc buna gore hareket eder, cihaz bunu algilamaz.
extern bool orthoActive[HR_SLOTS];
extern int orthoPhase[HR_SLOTS];             // 0=inactive, 1=Faz1 (yatarken), 2=Faz2 (ayaktayken)
extern unsigned long orthoPhaseStartMs[HR_SLOTS];

// O ANKI fazin biriktirme sayaclari - faz degisince (veya test bitince) sifirlanir.
extern float orthoBpmSum[HR_SLOTS];
extern int orthoBpmCount[HR_SLOTS];
extern float orthoRmssdSum[HR_SLOTS];
extern int orthoRmssdCount[HR_SLOTS];

// Test SONUCLARI - tamamlanan fazlarin ortalamalari, yeni test baslayana/
// resetSession'a kadar donuk kalir. -1 = o faz hic olculmedi/tamamlanmadi.
extern float orthoHr1[HR_SLOTS];
extern float orthoHr2[HR_SLOTS];
extern float orthoRmssdResult1[HR_SLOTS];
extern float orthoRmssdResult2[HR_SLOTS];

// O anki fazi Faz1 (yatarken) olarak baslatir - Faz2'ye ORTHO_PHASE_MS sonra
// otomatik gecer (bkz. TOAPERFORM.ino). hasFreshSignal false ise (guvenilir
// bpm yoksa) caller (WebRoutes) baslatmamali - HRR ile ayni desen.
void startOrthoTest(int slot);

extern unsigned long lastPeerBroadcastMs;  // ESP-NOW takim yayini icin son yayin zamani

// ---------------- "Bugun" tahmini (ACWR icin, bkz. Config.h ACWR notu) ----------------
// ESP32'de RTC/NTP yok - telefonun tarayicisi periyodik olarak GERCEK epoch
// saniyeyi ('ts' parametresiyle) gonderir, cihaz bunu alip millis() farkiyla
// ileri tasir. Hic senkron olmadiysa lastKnownEpochSec == 0 kalir.
extern unsigned long lastKnownEpochSec;
extern unsigned long lastKnownEpochAtMs;
void updateEpochSync(unsigned long epochSec);
// -1 = henuz hic senkron olmadi (ACWR gosterilmemeli).
long currentDayIndex();

// slot: HR_SLOTS araliginda olmali.
void setWarning(int slot, const char* msg, unsigned long durationMs);
void resetSession();
void formatTime(unsigned long sec, char* out, size_t outSize);
