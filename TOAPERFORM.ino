// =====================================================
// TOAPERFORM Athlete Heart Rate
// ESP32 + Polar Verity Sense (veya standart BLE Heart Rate Service uyumlu
// baska bir kol/gogus bandi)
//
// 2026-08: kapsam SADECE nabza daraltildi - GPS (hiz/mesafe/sprint/saha isi
// haritasi), barometre/sicrama ve Kariyer Karti/Yetenek/ACWR/Gelisim
// puanlamasi kullanici talebiyle kaldirildi. ESP-NOW Takim ozelligi (sadece
// nabiz/yorgunluk ozetiyle) geri eklendi.
//
// Bu dosya ince bir ORKESTRATOR: donanim baslatma (setup) ve ana dongu (loop)
// disinda mantik icermez. Gercek hesaplama/durum mantigi modullere tasindi:
//   Config.h              - pinler, sabitler, esik degerleri
//   PlayerMath.h/.cpp      - SAF: nabiz tabanli yorgunluk skoru (PC'de test edilir)
//   AlertOutput.h/.cpp      - titresim motoru deseni
//   SeasonStore.h/.cpp      - LittleFS (sezon pasaportu + antrenman gecmisi)
//   HeartRateHardware.h/.cpp - BLE (NimBLE) Heart Rate Service istemcisi
//   TeamNetwork.h/.cpp      - ESP-NOW takim (nabiz/yorgunluk) yayini
//   WebUi.h                - panel/rapor HTML govdesi
//   WebRoutes.h/.cpp        - HTTP handler'lari (glue katmani)
//   Globals.h/.cpp          - alt sistem ornekleri + paylasilan oturum durumu
//
// PC'de gercekten calisan unit testler icin: ..\TOAPERFORM_tests\test_main.cpp
// (Zig ile derlenir, Arduino derlemesini ETKILEMEZ - sketch klasorunun disinda).
// =====================================================
#include <WiFi.h>
#include <string.h>

#include "Globals.h"
#include "WebRoutes.h"

static IPAddress apIP(192, 168, 4, 1);
static const byte DNS_PORT = 53;

void setup() {
  Serial.begin(115200);
  delay(1000);

  alertOutput.begin();

  seasonStore.begin();
  rosterStore.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASS, WIFI_CHANNEL);

  dnsServer.start(DNS_PORT, "*", apIP);

  teamNetwork.begin();

  // NOT: BLE (nabiz), WiFi AP ve ESP-NOW ile AYNI radyoyu paylasir. WiFi/ESP-NOW
  // kurulumundan SONRA baslatildi ki bir sorun cikarsa once WiFi/panel/takim
  // islevleri ayakta olsun (bkz. HeartRateHardware.h).
  heartRateHardware.begin();

  registerWebRoutes();
  server.begin();

  resetSession();

  Serial.println("Sistem basladi.");
  Serial.print("WiFi: ");
  Serial.println(WIFI_AP_NAME);
  Serial.println("Sifre: 12345678");
  Serial.println("Adres: 192.168.4.1");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long now = millis();

  // ---------------- Nabiz (BLE Heart Rate - en fazla HR_SLOTS cihaz) ----------------
  heartRateHardware.poll(now);

  // Her turda (sadece yeni ornek geldiginde degil) yeniden hesaplanir - kol
  // bandi koldan cikarilip yeni ornek gelmeyi kesince "donuk" son BPM'in
  // gecerliymis gibi gorunmesini onlemek icin (bkz. HeartRateHardware.h
  // "SAHA BULGUSU" notu).
  for (int i = 0; i < HR_SLOTS; i++) {
    heartRateBpm[i] = heartRateHardware.bpm(i);
    heartRateContact[i] = heartRateHardware.sensorContact(i);
    heartRateSignalFresh[i] = heartRateHardware.hasFreshSignal(i, now);
    if (heartRateSignalFresh[i] && heartRateBpm[i] > sessionMaxHr[i]) sessionMaxHr[i] = heartRateBpm[i];
  }

  // ---------------- Uyari titresim ciktisi ----------------
  // Fiziksel titresim motoru/buzzer TEK - bu cihazin sahibi sayilan slot 0'in
  // riskine baglidir (bkz. Config.h HR_DEVICE_1_LABEL yorumu). Diger slotlarin
  // KRITIK durumu sadece panelde metin/renk olarak gorunur, titresim TETIKLEMEZ.
  alertOutput.updateCriticalRepeat(strcmp(riskStatus[0], "KRITIK") == 0);

  // ---------------- Takim / ESP-NOW ----------------
  // Roster'dan oyuncu atanmis (slotPlayerId != 0) HER slot icin AYRI bir ozet
  // yayinlanir - "deviceName" alani artik bu ESP32'nin sabit kimligi degil,
  // o slota atanmis OYUNCUNUN adidir (bkz. Globals.h). Atanmamis slotlar
  // yayinlanmaz - hub tarafinda "isimsiz" bir oyuncu gorunmesin diye.
  if (now - lastPeerBroadcastMs >= PEER_BROADCAST_MS) {
    lastPeerBroadcastMs = now;

    for (int i = 0; i < HR_SLOTS; i++) {
      if (slotPlayerId[i] == 0) continue;

      PeerSummary summary;
      strncpy(summary.deviceName, slotPlayerName[i], sizeof(summary.deviceName) - 1);
      summary.heartRateBpm = heartRateSignalFresh[i] ? heartRateBpm[i] : 0;
      summary.fatigueScore = fatigueScore[i];
      strncpy(summary.riskStatus, riskStatus[i], sizeof(summary.riskStatus) - 1);
      summary.hrZone = heartRateSignalFresh[i] ? hrZoneNow[i] : 0;
      summary.sessionSeconds = sessionSeconds;

      teamNetwork.broadcast(summary);
    }
  }

  // ---------------- Daha kaba/agir hesaplar (1 Hz) ----------------
  // Risk skoru ve debug ciktisi saniyede 1 kez calisir - yuksek frekansta
  // bunlari calistirmanin faydasi yok.
  static unsigned long lastCalcMs = 0;
  if (now - lastCalcMs >= 1000) {
    lastCalcMs = now;

    sessionSeconds = (now - sessionStartMs) / 1000;

    Serial.println("--------------------");
    char debugTimeBuf[16];
    formatTime(sessionSeconds, debugTimeBuf, sizeof(debugTimeBuf));
    Serial.print("Sure: ");
    Serial.println(debugTimeBuf);

    for (int i = 0; i < HR_SLOTS; i++) {
      // RR-interval/HRV (bkz. Config.h HR_RR_BUFFER_SIZE notu) - oyuncu
      // atanmasindan bagimsiz, salt donanimdan gelen bir deger.
      hrRrSupported[i] = heartRateHardware.rrSupported(i);
      if (hrRrSupported[i]) {
        uint16_t rrBuf[HR_RR_BUFFER_SIZE];
        int rrCount = heartRateHardware.rrIntervals(i, rrBuf, HR_RR_BUFFER_SIZE);
        PlayerMath::HrvMetrics hrv = PlayerMath::calculateHrv(rrBuf, rrCount);
        hrRmssdMs[i] = hrv.rmssdMs;
        hrSdnnMs[i] = hrv.sdnnMs;
        hrPnn50[i] = hrv.pnn50Pct;

        // HRV Taban Cizgisi (2026-08 ekleme, bkz. Config.h/RosterStore.h notu):
        // oturum boyunca GERCEK RMSSD orneklerini biriktirir - oturum sonunda
        // (WebRoutes handleReset) ortalamasi kisisel tabana (EWMA) katilir.
        // Sadece gecerli (>=2 RR ornegi olan, dolayisiyla rmssdMs>0 olan) turlar
        // sayilir.
        if (hrv.rmssdMs > 0) {
          hrRmssdSessionSum[i] += hrv.rmssdMs;
          hrRmssdSessionCount[i]++;
        }

        // Solunum Hizi Tahmini (2026-08 ekleme, bkz. Config.h/PlayerMath.h
        // BREATHING notu) - ayni RR tamponundan, ek bir okuma gerekmeden.
        breathingRateBpm[i] = PlayerMath::estimateBreathingRate(rrBuf, rrCount);
      }

      // Ortostatik Toparlanma Testi (2026-08 ekleme, bkz. Config.h/Globals.h
      // ORTHO notu) - panelden manuel baslatilir, iki faz (yatarken/ayaktayken)
      // SABIT SURELI ve otomatik ilerler. Guvenilir bpm/RMSSD sadece o an
      // gecerliyse fazin ortalamasina katilir (HRR'deki "sadece fresh sinyalde
      // olcum yapilir" ilkesiyle ayni).
      if (orthoActive[i]) {
        unsigned long orthoElapsed = now - orthoPhaseStartMs[i];

        if (heartRateSignalFresh[i]) {
          orthoBpmSum[i] += heartRateBpm[i];
          orthoBpmCount[i]++;
        }
        if (hrRrSupported[i] && hrRmssdMs[i] > 0) {
          orthoRmssdSum[i] += hrRmssdMs[i];
          orthoRmssdCount[i]++;
        }

        if (orthoElapsed >= ORTHO_PHASE_MS) {
          if (orthoPhase[i] == 1) {
            orthoHr1[i] = orthoBpmCount[i] > 0 ? (orthoBpmSum[i] / orthoBpmCount[i]) : -1;
            orthoRmssdResult1[i] = orthoRmssdCount[i] > 0 ? (orthoRmssdSum[i] / orthoRmssdCount[i]) : -1;

            orthoPhase[i] = 2;
            orthoPhaseStartMs[i] = now;
            orthoBpmSum[i] = 0;
            orthoBpmCount[i] = 0;
            orthoRmssdSum[i] = 0;
            orthoRmssdCount[i] = 0;
          } else if (orthoPhase[i] == 2) {
            orthoHr2[i] = orthoBpmCount[i] > 0 ? (orthoBpmSum[i] / orthoBpmCount[i]) : -1;
            orthoRmssdResult2[i] = orthoRmssdCount[i] > 0 ? (orthoRmssdSum[i] / orthoRmssdCount[i]) : -1;

            orthoActive[i] = false;  // test tamamlandi
          }
        }
      }

      // Kalp Hizi Toparlanmasi (HRR) testi - panelden manuel baslatilir (bkz.
      // Config.h HRR notu). Sadece guvenilir (fresh) bpm varken 1./2. dakika
      // olcumu YAPILIR - aksi halde donuk/eski bir deger yanlislikla "olcum"
      // sayilmasin diye o turda atlanir, bir sonraki turda tekrar denenir.
      if (hrrActive[i]) {
        unsigned long elapsed = now - hrrStartMs[i];
        if (hrrHr60[i] < 0 && elapsed >= HRR_MARK_1_MS && heartRateSignalFresh[i]) {
          hrrHr60[i] = heartRateBpm[i];
        }
        if (hrrHr120[i] < 0 && elapsed >= HRR_MARK_2_MS && heartRateSignalFresh[i]) {
          hrrHr120[i] = heartRateBpm[i];
          hrrActive[i] = false;  // test tamamlandi
        }
      }

      // Atanmamis slotlarda da BPM okunur/gosterilir (bkz. HeartRateHardware),
      // ama kisisel rekor olmadan yuzde/bolge/yorgunluk hesaplanamaz - bu
      // yuzden asagidaki blok, atanmamis slotu 0'da/NORMAL'de tutar.
      float personalMaxHr = 0;
      bool hasPlayer = (slotPlayerId[i] != 0);
      if (hasPlayer) {
        int idx = rosterStore.findIndexById(slotPlayerId[i]);
        if (idx >= 0) personalMaxHr = rosterStore.playerAt(idx).maxHrEver;
      }

      PlayerMath::FatigueInputs fi;
      float pctOfPersonalMaxHr = 0;

      // ---- Nabiz tabanli ic yuk birikimi (bkz. Config.h / PlayerMath.h notlari) ----
      // Sadece GUVENILIR sinyal varken (kol bandi koldan cikinca ASLA katkida
      // bulunmaz) VE bu slota atanmis oyuncunun kisisel rekoru yeterince
      // anlamliysa (PERSONAL_MIN_HR_SAMPLE) biriktirilir.
      if (hasPlayer && heartRateSignalFresh[i] && personalMaxHr >= PERSONAL_MIN_HR_SAMPLE) {
        pctOfPersonalMaxHr = (heartRateBpm[i] / personalMaxHr) * 100.0f;
        float hrExcess = pctOfPersonalMaxHr - HR_FATIGUE_BASELINE_PCT;
        if (hrExcess > 0) hrLoadAccum[i] += hrExcess;
      }
      fi.hrLoadAccum = hrLoadAccum[i];

      // ---- Nabiz bolgeleri (bkz. PlayerMath::hrZoneFromPercent) ----
      hrPctOfMax[i] = pctOfPersonalMaxHr;
      hrZoneNow[i] = PlayerMath::hrZoneFromPercent(pctOfPersonalMaxHr);
      if (hrZoneNow[i] >= 1 && hrZoneNow[i] <= 5) hrZoneSeconds[i][hrZoneNow[i] - 1]++;

      PlayerMath::RiskResult risk = PlayerMath::calculateFatigueRisk(fi);
      fatigueScore[i] = risk.score;
      strncpy(riskStatus[i], risk.status, sizeof(riskStatus[i]) - 1);
      riskStatus[i][sizeof(riskStatus[i]) - 1] = '\0';
      strncpy(riskColor[i], risk.colorHex, sizeof(riskColor[i]) - 1);
      riskColor[i][sizeof(riskColor[i]) - 1] = '\0';

      if (!hasPlayer) {
        if (millis() > warningUntilMs[i]) strcpy(lastWarning[i], "Bu bant icin oyuncu secilmedi");
      } else if (strcmp(risk.status, "NORMAL") == 0) {
        if (millis() > warningUntilMs[i]) {
          strcpy(lastWarning[i], heartRateSignalFresh[i] ? "Normal tempo" : "Nabiz sinyali bekleniyor");
        }
      } else if (strcmp(risk.status, "UYARI") == 0) {
        if (millis() > warningUntilMs[i]) {
          strcpy(lastWarning[i], "UYARI: Yorgunluk artiyor, tempoyu dusur");
        }
      } else {  // KRITIK
        strcpy(lastWarning[i], "KRITIK: Sakatlanma riski yuksek, dinlenme onerilir");
        warningUntilMs[i] = millis() + 3000;
      }

      fatigueTrend[i].addSample(fatigueScore[i]);

      // Bant hic tanimli degilse (Config.h'da MAC yok) satiri hic yazma -
      // once (herhangi bir Serial.print'ten ONCE) kontrol edilir, aksi halde
      // "Nabiz (BantN): " onegi bile bassiz kalirdi (SAHA BULGUSU, 2026-08).
      if (!heartRateHardware.slotEnabled(i)) continue;

      Serial.print("Nabiz (");
      Serial.print(hasPlayer ? slotPlayerName[i] : heartRateHardware.label(i));
      Serial.print("): ");
      if (heartRateSignalFresh[i]) {
        Serial.print(heartRateBpm[i]);
        Serial.println(" bpm");
      } else if (heartRateHardware.isConnected(i)) {
        Serial.println("Bagli ama sinyal yok (temas kaybi olabilir)");
      } else {
        Serial.println(heartRateHardware.isScanning() ? "Araniyor..." : "Bagli degil");
      }
      if (hasPlayer) {
        Serial.print("  Yorgunluk: ");
        Serial.print(fatigueScore[i]);
        Serial.print(" (");
        Serial.print(riskStatus[i]);
        Serial.println(")");
      }
    }
  }
}
