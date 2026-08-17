#pragma once
// =====================================================
// TOAPERFORM - HeartRateHardware.h
// Donanim adaptoru: Polar Verity Sense (veya standart BLE Heart Rate Service
// 0x180D yayinlayan herhangi bir gogus/kol bandi) ile BLE uzerinden baglanip
// anlik nabzi (BPM) okur. Bu, Bluetooth SIG'in standart "Heart Rate Profile"i
// - Polar'a ozel kapali bir protokol degil, bu yuzden Polar disinda 0x180D
// yayinlayan herhangi bir sensor de calisir.
//
// NimBLE-Arduino kullanilir (stok ESP32 BLE kutuphanesine gore ~100KB daha az
// RAM), cunku cihaz zaten WiFi AP + ESP-NOW calistiriyor - ayni radyoya bir de
// agir BLE yigini eklemek WiFi tarafinda kararsizliga yol acabilirdi.
//
// SAHA DOGRULAMASI (2026-07): Ilk denemede tarama sabit bir sure (5s) sonra
// durup periyodik olarak yeniden baslatiliyordu - bu, GERCEK donanimda birkac
// dongu icinde ESP32'yi crash/reboot dongusune sokuyordu (Serial ciktisi
// bozuluyordu, bkz. .cpp). SUREKLI tarama (hic durdurup yeniden baslatmadan)
// 20+ dakika kesintisiz test edildi ve sorun cikmadi - bu yuzden tasarim
// SUREKLI taramaya cevrildi: sadece eslesen cihaz bulunca (baglanmak icin)
// veya baglanti koparinca durur/yeniden baslar.
//
// SAHA DOGRULAMASI (2026-08): WiFi AP + BLE (surekli tarama + baglanti) AYNI
// ANDA calisirken interval==window (%100 duty-cycle) WiFi'nin DHCP gibi zaman
// hassas alisverislerini tamamen tikiyordu - bkz. .cpp begin() icindeki not.
// window'un interval'in altina indirilmesiyle (~%30 duty-cycle) cozuldu.
//
// SAHA BULGUSU (2026-07): Kol bandi koldan cikarilinca BPM degeri DUSMEDI -
// donuk kaldi. Sebep: cihaz temas kaybinca ya hic yeni notify GONDERMIYOR ya
// da eski/anlamsiz deger gonderiyor olabilir, ama bizim kod "son gelen deger"i
// sonsuza kadar gecerli sayiyordu (yeni ornek gelmedigi surece degismiyordu).
// Duzeltme: hasFreshSignal() eklendi - hem "son X ms icinde yeni ornek geldi
// mi" (bkz. HR_STALE_TIMEOUT_MS, Config.h) hem de "cihaz temas kaybi
// bildiriyor mu" kontrolunu yapar. Caller (TOAPERFORM.ino) BPM'i sadece bu
// true donunce "gecerli" saymali, aksi halde "sinyal yok" gostermeli.
//
// COKLU CIHAZ DESTEGI (2026-08): Ayni ESP32 en fazla SLOT_COUNT nabiz
// sensorune AYNI ANDA baglanir - bkz. Config.h HR_DEVICE_MAC[]/HR_DEVICE_LABEL[]
// icin MAC-bazli eslestirme ve geriye-uyumluluk notu. Slot ayrimi SIRAYA GORE
// (kim once eslesirse) DEGIL, MAC ADRESINE GORE yapilir - aksi halde bir
// baglanti kopup yeniden kuruldugunda iki oyuncunun nabzi birbirine karisirdi.
// Slot 0 DISINDAKI slotlar BILEREK bu cihazin kendi yorgunluk/bolge hesabina
// (hrZoneNow, hrLoadAccum, fatigueScore) KATILMAZ - hepsi roster'dan atanan
// KENDI oyuncusunun kisisel rekoruna gore AYRI AYRI hesaplanir (bkz. Globals.h/
// TOAPERFORM.ino), farkli kisilerin nabzi birbirine asla karismaz.
//
// SLOT_COUNT = 9 (2026-08, kullanici karari): ESP32 BLE denetleyicisinin
// donanim tavani 9 (bkz. NimBLE-Arduino/src/nimconfig.h
// CONFIG_BT_NIMBLE_MAX_CONNECTIONS, bu projede 3'ten 9'a cikarildi). BU SAYI
// ESP32 icin ASILAMAZ - daha fazla oyuncu icin (bkz. Config.h "24 oyuncu"
// notu) COKLU ESP32 birimi + hub mimarisi gerekir, tek cihaz sinirini
// yazilimla asmak MUMKUN DEGIL.
//
// SAHADA HENUZ DOGRULANMADI: WiFi AP + ESP-NOW + 3'ten fazla BLE baglantisinin
// AYNI ANDA stabil calistigi - 2 baglanti bile once kararsizdi (bkz. yukaridaki
// notlar), 9'a cikmak coexistence riskini ONEMLI OLCUDE artirir. Gercek pratik
// tavan (kac baglanti WiFi panelini bozmadan calisir) SADECE sahada, gercek
// donanimla test edilerek ogrenilebilir.
//
// RR-INTERVAL / HRV (2026-08): BLE HR Measurement karakteristiginde RR-interval
// alani OPSIYONELDIR (Flags biti 4) - cihaz gondermeyebilir, bkz. Config.h
// HR_RR_BUFFER_SIZE notu (Polar Sense'in bu modda gonderip gondermedigi
// SAHADA HENUZ DOGRULANMADI). Gelirse, her slot icin halka tamponda (bkz.
// HR_RR_BUFFER_SIZE) son degerler (ms) tutulur - rrIntervals() ile chronological
// (eskiden yeniye) sirada okunur, RMSSD hesabi PlayerMath::calculateRmssd()'de.
// =====================================================
#include <stddef.h>
#include <stdint.h>

class HeartRateHardware {
public:
  static const int SLOT_COUNT = 9;

  void begin();

  // loop() icinde her turda cagrilmali. Baglanmamis (etkin) slot varsa
  // surekli tarar; bir tarama sonucu baglanmayi gerektiriyorsa gercek
  // connect() islemini de (NimBLE callback'inden degil) burada, ana loop
  // baglaminda yapar.
  void poll(unsigned long nowMs);

  // slot: 0..SLOT_COUNT-1. Config.h'daki HR_DEVICE_MAC[slot]/HR_DEVICE_LABEL[slot]'e
  // karsilik gelir. Slot Config.h'da MAC'siz (devre disi) birakildiysa bpm()
  // hep 0, hasFreshSignal() hep false doner.
  int bpm(int slot) const;
  bool sensorContact(int slot) const;
  bool contactSupported(int slot) const;
  bool isConnected(int slot) const;
  bool slotEnabled(int slot) const;
  const char* label(int slot) const;

  // Bu cihazin/modun RR-interval GONDERDIGI en az bir kere gozlemlendiyse true
  // (bkz. Config.h notu - opsiyonel bir alan, cihaz hic gondermeyebilir).
  bool rrSupported(int slot) const;

  // outBuf'a o slotun son RR degerlerini (ms, ESKIDEN YENIYE sirali) yazar,
  // yazilan adedi doner (en fazla maxCount, gercekte varsa daha az olabilir).
  int rrIntervals(int slot, uint16_t* outBuf, int maxCount) const;

  bool isScanning() const;

  // BPM degerinin "su an gecerli" sayilip sayilamayacagi: baglanti var, son
  // ornek HR_STALE_TIMEOUT_MS icinde geldi VE cihaz temas kaybi bildirmiyor.
  // Bu false donerse caller son bilinen BPM'i GOSTERMEMELI (donuk deger yerine
  // "sinyal yok" gibi bir gösterge kullanmali).
  bool hasFreshSignal(int slot, unsigned long nowMs) const;
};
