#include "HeartRateHardware.h"
#include "Config.h"
#include <NimBLEDevice.h>
#include <string.h>

// Standart Bluetooth SIG Heart Rate Service / Characteristic UUID'leri.
static const NimBLEUUID HR_SERVICE_UUID((uint16_t)0x180D);
static const NimBLEUUID HR_CHAR_UUID((uint16_t)0x2A37);

// Slot basina tum baglanti/olcum durumu. volatile alanlar NimBLE'in kendi
// task'inden (notify/disconnect callback'leri) yazilir, ana loop poll()
// icinde okur - basit int/bool okuma-yazmalari icin ayri bir mutex kullanilmadi.
struct HrSlot {
  const char* label = "";
  bool enabled = false;          // Config.h'da MAC tanimli mi (veya legacy modda slot 0)
  // MAC, NimBLEAddress yerine DUZ METIN olarak karsilastirilir (sadece
  // deger baytlari, adres TIPI - public/random - gormezden gelinir): Config.h'a
  // Serial log'da gorunen MAC'i oldugu gibi kopyala-yapistir yeterli olsun
  // diye. NimBLEAddress::operator== hem deger hem TIP alanini karsilastirir -
  // saatin gercek adres tipini Config.h'a ayrica yazdirmaya gerek birakmamak
  // icin bilerek NimBLEAddress kullanilmadi.
  const char* configuredMac = "";
  NimBLEClient* client = nullptr;

  volatile bool connected = false;
  volatile int latestBpm = 0;
  volatile bool latestContact = false;
  volatile bool latestContactSupported = false;
  volatile bool hasNewSample = false;
  volatile unsigned long lastSampleMs = 0;
  volatile int lastDistinctBpm = -1;
  volatile unsigned long lastChangeMs = 0;
};

static HrSlot s_slots[HeartRateHardware::SLOT_COUNT];

// GERIYE UYUMLULUK (bkz. Config.h): Config.h'da HIC MAC tanimlanmadiysa
// (mevcut tek-saat kurulumlari) eski davranis aynen korunur - 0x180D
// yayinlayan ILK cihaza slot 0 uzerinden baglanilir, MAC kontrolu YAPILMAZ.
// MAC alanlarindan en az biri doldurulunca bu kapanir ve sadece belirtilen
// MAC'lere baglanilir.
static bool s_legacyMode = false;

// NimBLE'in tarama sonucu callback'i (onResult), asil connect() islemini
// KENDI ICINDE yapmaz - sadece bulunan cihazi kopyalayip bayrak set eder;
// gercek baglanma poll() icinde (ana loop baglaminda) yapilir. Tek seferde
// EN FAZLA BIR baglanma denemesi beklemede tutulur (birden fazla cihaz ayni
// anda bulunursa digerleri bir sonraki tarama turunde yakalanir).
static bool s_shouldConnect = false;
static NimBLEAdvertisedDevice* s_pendingDevice = nullptr;
static int s_pendingSlot = -1;

// SAHADA DOGRULANDI (bkz. HeartRateHardware.h): ilk surumde tarama, sabit bir
// sure sonra durdurulup periyodik olarak yeniden baslatiliyordu (start/stop
// dongusu) - bu, gercek donanimda birka ONiyeti icinde ESP32'yi CRASH/REBOOT
// dongusune sokuyordu (Serial cikisi bozuluyordu). SUREKLI tarama (duration=0,
// hic durdurup yeniden baslatmadan) 20+ dakika kesintisiz test edildi, sorun
// cikmadi. Bu yuzden: tarama SADECE bir kez baslar (begin() icinde), sadece
// eslesen cihaz bulunca (baglanmak icin) veya baglanti koparinca durur/yeniden
// baslar - periyodik "yeniden dene" dongusu YOK.
static bool s_needRescan = false;

// ---------------- Nabiz olcumu ayristirma (BLE HR profili, GATT spec) ----------------
// Flags byte (data[0]): bit0 = deger formati (0=uint8, 1=uint16), bit1 = sensor
// temas algilandi mi, bit2 = temas bilgisi destekleniyor mu, bit3 = enerji
// harcamasi alani var mi, bit4 = RR-interval alani var mi (HRV icin - su an
// kullanilmiyor, gelecekte eklenebilir).
static void parseHrMeasurement(int slot, const uint8_t* data, size_t length) {
  if (length < 2) return;

  uint8_t flags = data[0];
  bool is16bit = flags & 0x01;
  bool contactSupported = flags & 0x04;
  bool contactDetected = flags & 0x02;

  int bpm;
  if (is16bit) {
    if (length < 3) return;
    bpm = data[1] | (data[2] << 8);
  } else {
    bpm = data[1];
  }

  HrSlot& s = s_slots[slot];
  s.latestBpm = bpm;
  s.latestContactSupported = contactSupported;
  s.latestContact = contactSupported ? contactDetected : false;
  s.hasNewSample = true;
  s.lastSampleMs = millis();

  // SAHA BULGUSU (2026-07, bkz. HeartRateHardware.h): Polar Sense (Verity
  // Sense'in bu yayin modundaki BLE adi) HICBIR ZAMAN contactSupported=true
  // bildirmiyor (flags hep 0b00000000 olculdu) - yani standart BLE HR
  // profilinin "temas koptu" mekanizmasi bu cihaz/modda KULLANILAMIYOR. Kol
  // cikarilinca cihaz notify GONDERMEYI KESMIYOR, "donan/yavas surunen" bir
  // deger yollamaya devam ediyor. Bunun icin ek bir sezgisel kontrol: ayni BPM
  // degeri HR_STUCK_TIMEOUT_MS'den uzun sure DEGISMEDEN kalirsa "supheli/
  // durgun" sayilir (bkz. hasFreshSignal). Bu KESIN bir tespit DEGIL - gercek
  // dinlenme nabzi da bazen birkac saniye ayni deger okuyabilir, sadece "cok
  // uzun sure hic degismedi" durumunu yakalar.
  if (bpm != s.lastDistinctBpm) {
    s.lastDistinctBpm = bpm;
    s.lastChangeMs = s.lastSampleMs;
  }
}

// TEK, PAYLASILAN disconnect callback'i: NimBLE'in onDisconnect'i HANGI
// NimBLEClient'in koptugunu (pClient) zaten parametre olarak veriyor, bu
// yuzden slot basina ayri bir callback SINIFI/ORNEGI gerekmiyor - pClient
// pointer'i s_slots[].client ile eslestirip dogru slotu buluyoruz. Bu, N
// slota (SLOT_COUNT degisse bile) hic ek kod gerektirmeden olcekleniyor.
class HrClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    int slot = -1;
    for (int i = 0; i < HeartRateHardware::SLOT_COUNT; i++) {
      if (s_slots[i].client == pClient) { slot = i; break; }
    }
    if (slot < 0) return;

    HrSlot& s = s_slots[slot];
    Serial.print("[BLE] Nabiz sensoru (");
    Serial.print(s.label);
    Serial.print(") baglantisi koptu, reason=");
    Serial.println(reason);
    s.connected = false;
    s.hasNewSample = false;
    s.lastSampleMs = 0;
    s.lastDistinctBpm = -1;
    s.lastChangeMs = 0;
    s_needRescan = true;
  }
};
static HrClientCallbacks s_clientCallbacks;

static bool allEnabledSlotsConnected() {
  for (int i = 0; i < HeartRateHardware::SLOT_COUNT; i++) {
    if (s_slots[i].enabled && !s_slots[i].connected) return false;
  }
  return true;
}

// Bulunan cihazin hangi (henuz bagli olmayan, etkin) slota karsilik geldigini
// belirler; eslesme yoksa -1 doner. Legacy modda MAC kontrolu yapilmaz -
// sadece bos (henuz bagli olmayan) slot 0'a baglanir (eski tek-cihaz davranisi).
static int matchSlotForDevice(const NimBLEAdvertisedDevice* device) {
  if (s_legacyMode) {
    return s_slots[0].connected ? -1 : 0;
  }
  for (int i = 0; i < HeartRateHardware::SLOT_COUNT; i++) {
    if (!s_slots[i].enabled || s_slots[i].connected) continue;
    if (strcasecmp(device->getAddress().toString().c_str(), s_slots[i].configuredMac) == 0) return i;
  }
  return -1;
}

class HrScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* device) override {
    if (s_shouldConnect) return;  // bir baglanma denemesi zaten beklemede
    if (!device->isAdvertisingService(HR_SERVICE_UUID)) return;

    int slot = matchSlotForDevice(device);
    if (slot < 0) return;

    Serial.print("[BLE] Nabiz servisi yayinlayan cihaz bulundu (");
    Serial.print(s_slots[slot].label);
    Serial.print("): ");
    Serial.print(device->getAddress().toString().c_str());
    Serial.print(" \"");
    Serial.print(device->haveName() ? device->getName().c_str() : "?");
    Serial.println("\"");

    NimBLEDevice::getScan()->stop();
    s_pendingDevice = new NimBLEAdvertisedDevice(*device);
    s_pendingSlot = slot;
    s_shouldConnect = true;
  }
  void onScanEnd(const NimBLEScanResults& /*results*/, int reason) override {
    Serial.print("[BLE] Tarama durdu, reason=");
    Serial.println(reason);
  }
};
static HrScanCallbacks s_scanCallbacks;

// Tarama sirasinda bulunan cihaza gercekten baglanip Heart Rate Service/
// Characteristic'ini kesfeder ve notify'a abone olur. Herhangi bir adimda
// basarisiz olursa sessizce false doner - caller s_needRescan set ederek
// taramayi yeniden baslatir (kalici bir hata durumu olarak ele alinmaz).
// Notify callback'i BURADA (baglanti aninda) slot indeksini YAKALAYAN bir
// lambda olarak olusturulur - NimBLE'in subscribe() imzasi std::function
// kabul ettigi icin (bkz. NimBLERemoteCharacteristic.h) slot basina ayri bir
// free function/dizi gerekmiyor, N slota otomatik olcekleniyor.
static bool connectToDevice(int slot, NimBLEAdvertisedDevice* device) {
  HrSlot& s = s_slots[slot];
  if (s.client == nullptr) {
    s.client = NimBLEDevice::createClient();
    s.client->setClientCallbacks(&s_clientCallbacks, false);
  }

  if (!s.client->connect(device)) return false;

  NimBLERemoteService* svc = s.client->getService(HR_SERVICE_UUID);
  if (svc == nullptr) {
    s.client->disconnect();
    return false;
  }

  NimBLERemoteCharacteristic* chr = svc->getCharacteristic(HR_CHAR_UUID);
  if (chr == nullptr || !chr->canNotify()) {
    s.client->disconnect();
    return false;
  }

  chr->subscribe(true, [slot](NimBLERemoteCharacteristic* /*chr*/, uint8_t* data, size_t length, bool /*isNotify*/) {
    parseHrMeasurement(slot, data, length);
  });
  return true;
}

static void startContinuousScan() {
  if (allEnabledSlotsConnected()) return;  // aranacak bir sey kalmadi
  // Yerel bir bayrak yerine kutuphanenin GERCEK tarama durumunu sorulur -
  // aksi halde (SAHA BULGUSU) bir onceki tarama/baglanti dongusunde bu
  // bayrak gercek durumdan kopabiliyor ve tarama bir daha HICBIR ZAMAN
  // yeniden baslamiyordu (isScanning() hep "true" donup burayi atlıyordu,
  // gercekte kutuphanede aktif tarama olmadigi halde).
  if (NimBLEDevice::getScan()->isScanning()) return;
  bool started = NimBLEDevice::getScan()->start(0, false);
  Serial.print("[BLE] Surekli tarama baslatma denemesi, sonuc=");
  Serial.println(started ? "BASARILI" : "BASARISIZ");
  s_needRescan = !started;  // basarisizsa bir sonraki poll()'da tekrar denenir
}

void HeartRateHardware::begin() {
  bool anyMacSet = false;
  for (int i = 0; i < SLOT_COUNT; i++) {
    if (HR_DEVICE_MAC[i] != nullptr && HR_DEVICE_MAC[i][0] != '\0') { anyMacSet = true; break; }
  }
  s_legacyMode = !anyMacSet;

  for (int i = 0; i < SLOT_COUNT; i++) {
    s_slots[i].label = HR_DEVICE_LABEL[i];
    s_slots[i].configuredMac = HR_DEVICE_MAC[i];
    bool macSet = HR_DEVICE_MAC[i] != nullptr && HR_DEVICE_MAC[i][0] != '\0';
    s_slots[i].enabled = s_legacyMode ? (i == 0) : macSet;  // eski tek-cihaz davranisi: sadece slot 0, MAC kontrolu yok
  }

  NimBLEDevice::init("");

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&s_scanCallbacks, false);
  scan->setActiveScan(true);
  // SAHADA DOGRULANDI (2026-08): interval==window (%100 duty-cycle, hic bosluksuz
  // tarama) WiFi+BLE coexistence zaman paylasimini tamamen BLE'ye kaydiriyordu -
  // telefon WiFi AP'ye assoc oluyordu ama DHCP hicbir zaman tamamlanmiyordu (IP
  // asla atanmadi). window'u interval'in altina indirmek (burada ~%30 duty-cycle)
  // WiFi'ye DHCP gibi zaman-hassas alisverisler icin yeterli radyo zamani birakiyor,
  // BLE taramasi surekli olmaya devam ediyor (sadece daha az agresif). NOT: bu
  // oran 1-2 baglantiyla dogrulandi, SLOT_COUNT=9'a cikildiginda (bkz.
  // HeartRateHardware.h) daha fazla baglanti kurulacaksa bu deger sahada
  // yeniden gozden gecirilmeli.
  scan->setInterval(160);
  scan->setWindow(48);

  startContinuousScan();
}

void HeartRateHardware::poll(unsigned long /*nowMs*/) {
  if (s_shouldConnect) {
    s_shouldConnect = false;
    int slot = s_pendingSlot;
    bool ok = connectToDevice(slot, s_pendingDevice);
    delete s_pendingDevice;
    s_pendingDevice = nullptr;
    s_pendingSlot = -1;
    s_slots[slot].connected = ok;
    // Basarisizsa OLAGAN sekilde yeniden taranir. Basariliysa da: eger HALA
    // bagli olmayan baska bir etkin slot varsa tarama devam etmeli - aksi
    // halde bir saat baglaninca tarama bir daha HIC baslamiyor, digerleri
    // asla bulunamiyor.
    if (!ok || !allEnabledSlotsConnected()) s_needRescan = true;
  }

  if (s_needRescan) {
    s_needRescan = false;
    startContinuousScan();
  }
}

int HeartRateHardware::bpm(int slot) const { return s_slots[slot].latestBpm; }
bool HeartRateHardware::sensorContact(int slot) const { return s_slots[slot].latestContact; }
bool HeartRateHardware::contactSupported(int slot) const { return s_slots[slot].latestContactSupported; }
bool HeartRateHardware::isConnected(int slot) const { return s_slots[slot].connected; }
bool HeartRateHardware::slotEnabled(int slot) const { return s_slots[slot].enabled; }
const char* HeartRateHardware::label(int slot) const { return s_slots[slot].label; }
bool HeartRateHardware::isScanning() const { return NimBLEDevice::getScan()->isScanning(); }

bool HeartRateHardware::hasFreshSignal(int slot, unsigned long nowMs) const {
  const HrSlot& s = s_slots[slot];
  if (!s.connected || s.lastSampleMs == 0) return false;
  if (nowMs - s.lastSampleMs > HR_STALE_TIMEOUT_MS) return false;
  if (s.latestContactSupported && !s.latestContact) return false;
  if (s.lastChangeMs != 0 && nowMs - s.lastChangeMs > HR_STUCK_TIMEOUT_MS) return false;
  return true;
}
