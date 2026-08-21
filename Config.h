#pragma once
// =====================================================
// TOAPERFORM - Config.h
// Donanim pinleri, ag kimlik bilgileri ve tum sabit esik degerleri.
// Bu dosya sadece sabitler icerir, mantik icermez - mantigi degistirmeden
// bir esigi ayarlamak icin tek bakilacak yer burasi olsun diye.
//
// 2026-08: kapsam daraltildi - artik SADECE nabiz (BLE Heart Rate) var.
// GPS (hiz/mesafe/sprint/isi haritasi), barometre/sicrama, ESP-NOW takim
// ozellikleri ve Kariyer Karti/Yetenek/ACWR/Gelisim puanlamasi kullanici
// talebiyle kaldirildi.
// =====================================================

// ---------------- Alert Output (titresim motoru / buzzer) ----------------
// NOT: GPIO 4 bu projede kullanilmayan bir pin olarak secildi, ama kartinizda
// baska bir amacla ayrilmis olabilir - kendi kart semanizde bos oldugunu
// dogrulayin. Titresim motoru GPIO'ya DOGRUDAN baglanmamali; bir NPN
// transistor (or. 2N2222) + ters akim diyotu ile suruculmelidir, aksi halde
// motor geri-EMF'i ESP32 pinine zarar verebilir.
#define ALERT_PIN 4
static const unsigned long ALERT_PULSE_MS = 400;
static const unsigned long ALERT_PULSE_GAP_MS = 250;  // desendeki titresimler arasi bosluk
static const unsigned long ALERT_REPEAT_MS = 30000;   // kritik durum surerken tekrar uyari araligi

// ---------------- WiFi ----------------
static const char* const WIFI_AP_NAME = "TOAPERFORM";
static const char* const WIFI_AP_PASS = "12345678";
static const int WIFI_CHANNEL = 1;
static const char* const DEVICE_NAME = "Oyuncu1";

// ---------------- Takim / ESP-NOW ----------------
// Sahada birden fazla TOAPERFORM cihazi calisirken her oyuncu kendi bagimsiz
// WiFi-AP'sini ve panelini calistirmaya devam eder (tek cihazlik kullanim hic
// degismez). Ek olarak her cihaz ESP-NOW ile (yonlendirici/internet gerekmeden,
// dogrudan cihazdan cihaza, ayni WiFi kanali uzerinden) kompakt bir nabiz/
// yorgunluk ozeti yayinlar. HUB olarak isaretlenen cihaz bu yayinlari dinleyip
// kendi panelinde bir "Takim" bolumu olarak toplar.
//
// Birden fazla cihaz flashlarken HER CIHAZA FARKLI bir DEVICE_NAME verin ve
// SADECE BIR cihazda DEVICE_IS_HUB = true birakin.
static const bool DEVICE_IS_HUB = true;
static const unsigned long PEER_BROADCAST_MS = 2000;
static const unsigned long PEER_TIMEOUT_MS = 15000;
static const int MAX_PEERS = 8;

// ---------------- Sunucuya Gonderim (toaperform.com) ----------------
// Cihazin kendisi internete cikmiyor (saha wifi'si yok/guvenilmez) - panel
// telefonda acikken, telefonun tarayicisi (WiFi ile cihaza, mobil veriyle
// internete AYNI ANDA baglanabildigi icin) bu adrese dogrudan JS fetch() ile
// POST atar. Bu anahtar toa-app'teki GPS_DEVICE_SHARED_KEY ile AYNI olmali
// (bkz. .env.local / sunucudaki .env) - degismezse istekler 401 doner.
// NOT: isimlendirme ("GPS_...") tarihsel - sunucu tarafindaki anahtar/route
// ismiyle uyumu bozmamak icin GPS kaldirildiktan sonra da DEGISTIRILMEDI.
static const char* const GPS_UPLOAD_URL = "https://toaperform.com/api/gps/ingest";
static const char* const GPS_DEVICE_KEY = "97680d1bf997a791f705880c69c5773650698a8ea6f16afd";

// ---------------- Session History (LittleFS) ----------------
static const char* const HISTORY_FILE = "/history.jsonl";
static const int MAX_HISTORY_ENTRIES = 20;
static const unsigned long MIN_HISTORY_SESSION_SECONDS = 30;

// ---------------- Season Passport (LittleFS) ----------------
static const char* const SEASON_FILE = "/season.dat";

// ---------------- Ongorucu Yorgunluk Uyarisi ----------------
static const int FATIGUE_TREND_WINDOW = 120;       // saniye (2 dakika), 1 ornek/saniye
static const int FATIGUE_TREND_MIN_SAMPLES = 30;   // guvenilir egim icin en az bu kadar saniye veri

// ---------------- Nabiz (BLE Heart Rate - Polar Verity Sense uyumlu) ----------------
// Standart Bluetooth SIG Heart Rate Service (0x180D) kullanilir - Polar'a ozel
// kapali bir protokol degil, bu yuzden 0x180D yayinlayan baska sensorler de
// calisir. Tarama SUREKLI calisir (periyodik durdur/baslat DEGIL - bkz.
// HeartRateHardware.h/.cpp'deki "SAHA DOGRULAMASI" notu: durdur/baslat dongusu
// gercek donanimda crash'e yol aciyordu).
// Sinyal tazelik esigi: bu sureden uzun zamandir yeni ornek gelmediyse BPM
// "eski/gecersiz" sayilir (bkz. hasFreshSignal - kol bandi koldan cikarilinca
// donuk deger gosterilmesi sorununu cozmek icin eklendi).
static const unsigned long HR_STALE_TIMEOUT_MS = 2000;
// "Donuk deger" esigi: Polar Sense (bkz. HeartRateHardware.cpp SAHA BULGUSU)
// temas kaybini BLE'nin standart mekanizmasiyla bildirmiyor - notify GELMEYE
// devam ediyor ama deger DEGISMEYEBILIYOR. Ayni BPM bu sureden UZUN
// degismeden kalirsa supheli/durgun sayilir. KESIN degil - sadece cok uzun
// sabit kalmayi yakalar. NOT (kullanici ile 2026-07 saha testinde ayarlandi):
// 2sn ve 3.5sn denendi - ikisi de takiliyken bile yanlis "sinyal yok"
// titremesine yol aciyordu (cihazin kendi degeri dogal olarak birkac saniye
// sabit kalabiliyor). 8000ms'e cikarildi CUNKU bu deger sadece EKRAN
// gostergesi icin degil, yorgunluk/risk hesabina (PlayerMath) da giriyor -
// o hesap icin ONCELIK hiz degil dogruluk: ekranda birkac saniyelik titreme
// zararsizken, yorgunluk skoruna yanlis/sahte bir BPM karismasi yanlis bir
// antrenman kararina yol acabilir. ONEMLI SINIR: cihaz gercek temas biti
// vermedigi icin (bkz. HeartRateHardware.cpp SAHA BULGUSU) bu, "deger
// DEGISIYOR ama gercekte kol/parmak temasi YOK" durumunu HALA yakalayamaz -
// sadece TAM SABIT kalan degerleri yakalar. KESIN bir cozum degil, sahada
// gorsel kontrolle birlikte kullanilmali.
static const unsigned long HR_STUCK_TIMEOUT_MS = 8000;

// ---------------- RR-Interval / HRV (RMSSD) ----------------
// BLE Heart Rate Measurement karakteristiginde (0x2A37) RR-interval alani
// OPSIYONELDIR (Flags biti 4) - cihaz gondermeyebilir. Polar Sense'in bu
// yayin modunda gonderip gondermedigi HENUZ SAHADA DOGRULANMADI (bkz.
// HeartRateHardware.cpp SAHA BULGUSU notlariyla ayni belirsizlik turu -
// contactSupported de benzer sekilde bu moddan hic gelmiyordu).
//
// ONEMLI METODOLOJIK SINIR: burada hesaplanan RMSSD, oyuncu HAREKET
// HALINDEYKEN (kosarken/antrenmandayken) surekli guncellenir - bu, HRV
// literaturundeki standart "dinlenme/toparlanma HRV'si" (sabah, 60-120sn
// hareketsiz olcum) ile AYNI SEY DEGILDIR. Egzersiz sirasinda RR degiskenligi
// solunum ve efor tarafindan domine edilir, otonom toparlanma durumunu
// yansitmaz. Bu deger sadece ANLIK/gostergesel bir metrik olarak sunulmali -
// panelde bu net belirtilmeli, "toparlanma skoru" gibi sunulmamali.
static const int HR_RR_BUFFER_SIZE = 40;  // slot basina tutulan son RR degeri sayisi (~halka tampon)

// ---------------- Kalp Hizi Toparlanmasi (HRR) ----------------
// Kaynak: Cole CR ve ark., N Engl J Med 2000. Efor sonrasi 1. ve 2. dakikada
// nabzin ne kadar dustugunu olcer - basit ama literatur-destekli bir
// kardiyovaskuler toparlanma gostergesi. Cihaz "efor bitti" anini GPS/
// ivmeolcer olmadan otomatik ALGILAYAMAZ - bu yuzden panelden MANUEL
// tetiklenir ("Testi Baslat"), o andaki bpm HR0 olarak alinir. NOT: Cole ve
// ark.'daki dusuk-HRR1 = artmis mortalite riski iliskisi GENEL POPULASYONDA
// kurulmustur, sporcu populasyonuna DOGRUDAN tasinamaz - bu yuzden bir risk
// bandi/siniflandirmasi SUNULMAZ, sadece ham dusus degeri (bpm) gosterilir.
static const unsigned long HRR_MARK_1_MS = 60000;   // 1. dakika
static const unsigned long HRR_MARK_2_MS = 120000;  // 2. dakika

// ---------------- Ortostatik Toparlanma Testi (Polar Recovery Pro tarzi) ----------------
// 2026-08 (ekleme): HRR ile AYNI felsefe - manuel tetiklenir, HAM deger (bpm/ms)
// gosterilir, klinik bir skor/risk bandi URETILMEZ (bkz. yukaridaki HRR notu -
// Cole ve ark. esikleri genel populasyonda kurulmus, sporcuya dogrudan
// tasinamaz; ortostatik testin kendi standart bir "iyi/kotu" esigi spor
// bilimi literaturunde HENUZ net degil, bu yuzden burada da sadece HAM
// deger/fark sunulur). HRR'nin aksine cihaz "efor bitti" gibi bir ani
// algilamiyor - iki faz (Faz1: yatarken/otururken, Faz2: ayaktayken) SABIT
// SURELI ve OTOMATIK zamanlanir. Panel faz gecisinde "AYAGA KALK" uyarisi
// gosterir - oyuncu/koc buna gore hareket eder, cihaz fiziksel duruma
// (yatik/ayakta) hic bakmaz, sadece zamanlayici kullanir.
static const unsigned long ORTHO_PHASE_MS = 120000;  // her faz (yatarken/ayaktayken) 2 dakika

// ---------------- Solunum Hizi Tahmini (RR-interval osilasyonu) ----------------
// 2026-08 (ekleme): Polar/Firstbeat'in Training Load raporlarinda kullandigi
// teknige benzer sekilde, RR-interval dizisindeki solunum kaynakli (RSA -
// respiratuar sinus aritmisi) salinimdan nefes/dakika tahmin edilir - ek
// donanim gerekmez, mevcut RR-interval tamponundan (bkz. HR_RR_BUFFER_SIZE)
// turetilir. Yontem: RR dizisinin ortalamasini YUKARI kesen (rising
// mean-crossing) sayisi / toplam sure - FFT gerektirmeyen, hafif bir yontem
// (bkz. PlayerMath::calculateHrv ile ayni "FFT yok" tercihi). ONEMLI SINIR:
// RSA yogun egzersiz sirasinda solunum/efor tarafindan bastirilir - bu tahmin
// en cok DINLENIK/hafif tempoda anlamlidir, yuksek nabizda guvenilirligi
// dusuktur (panelde bu belirtilmeli, "kesin" bir olcum gibi sunulmamali).
// Makul araligin (asagida) DISINDA cikan sonuc gurultu sayilir ve 0 (gecersiz)
// doner.
static const int BREATHING_MIN_RR_SAMPLES = 10;  // bu kadar RR ornegi olmadan tahmin yapilmaz
static const float BREATHING_MIN_VALID_BPM = 6.0f;   // bu altindaki sonuc gurultu sayilir
static const float BREATHING_MAX_VALID_BPM = 70.0f;  // bu ustundeki sonuc gurultu sayilir

// ---------------- Nabiz - Coklu Bant Destegi ----------------
// Ayni ESP32 en fazla HR_DEVICE_SLOTS (=HeartRateHardware::SLOT_COUNT, bkz.
// HeartRateHardware.h - IKI SABIT AYNI KALMALI) nabiz sensorune AYNI ANDA
// baglanir. 9, ESP32 BLE denetleyicisinin donanim tavanidir (bkz.
// NimBLE-Arduino/src/nimconfig.h CONFIG_BT_NIMBLE_MAX_CONNECTIONS - bu
// kurulumda 3'ten 9'a cikarildi) - YAZILIMLA daha fazla ARTIRILAMAZ, daha
// fazla oyuncu icin coklu ESP32 birimi + hub mimarisi gerekir (bkz. proje
// notlari, "24 oyuncu" hedefi).
//
// SAHADA HENUZ DOGRULANMADI: WiFi AP + ESP-NOW + 3'ten fazla BLE baglantisinin
// AYNI ANDA stabil calistigi - 2 baglanti bile once kararsizdi (bkz.
// HeartRateHardware.h/.cpp notlari). Gercek pratik tavan sahada denenerek
// bulunmali; sorun cikarsa HR_DEVICE_MAC[] icindeki fazla slotlari bos
// birakip fiilen kullanilan bant sayisina geri donulebilir.
//
// Hangi fiziksel bandin hangi slota (etikete) karsilik geldigi SIRAYA GORE
// (kim once eslesirse) DEGIL, MAC ADRESINE GORE SABIT belirlenir - boylece bir
// baglanti kopup yeniden kuruldugunda iki oyuncunun nabzi birbirine KARISMAZ
// (bkz. HeartRateHardware.cpp). MAC adresini ogrenmek icin: yeni bandi acip
// ESP32'yi Serial Monitor acikken calistirin - "[BLE] Nabiz servisi yayinlayan
// cihaz bulundu (...): AA:BB:CC:DD:EE:FF" satirinda MAC yazar (buyuk/kucuk harf
// onemli degil).
//
// GERIYE UYUMLULUK: TUM MAC'ler bos ("") birakilirsa eski davranis AYNEN
// korunur - ilk bulunan 0x180D cihazina (hangisi olursa) SADECE slot 0
// uzerinden baglanilir, tek-bant kurulumu hic bozulmaz. MAC alanlarindan EN AZ
// BIRI doldurulunca sistem "kesin eslesme" moduna gecer: sadece belirtilen
// MAC'lere baglanir, bos birakilan slotlar devre disi kalir.
//
// LABEL alanlari bir OYUNCU ADI DEGIL - sadece o BLE slotuna henuz roster'dan
// kimse atanmadiginda panelde gorunecek jenerik bant etiketi (bkz.
// RosterStore.h, asagidaki "Oyuncu Roster'i" bolumu). Bantlar ortak havuzdan
// dagitildigi icin gercek kimlik (isim + kisisel nabiz rekoru) MAC'e degil,
// panelden secilen roster kaydina bagli - boylece bir oyuncu farkli gunlerde
// farkli fiziksel bant takabilir, kendi rekoru/bolgeleri hep dogru kisiye
// islenir. Slot 0 DISINDAKILER (Bant2..Bant9) BILEREK bu cihazin kendi
// yorgunluk skoruna KATILMAZ - hepsi roster'dan atanan KENDI oyuncusuna gore
// AYRI hesaplanir (bkz. Globals.h, TOAPERFORM.ino).
static const int HR_DEVICE_SLOTS = 9;  // HeartRateHardware::SLOT_COUNT ile AYNI olmali
static const char* const HR_DEVICE_MAC[HR_DEVICE_SLOTS] = {
  "24:ac:ac:07:45:27",  // slot 0 - Polar Sense 0745273E
  "24:ac:ac:0c:9d:9c",  // slot 1 - Polar Sense 0C9D9C34
  "", "", "", "", "", "", ""  // slot 2..8 - TODO: yeni bant eklendikce MAC yazin
};
static const char* const HR_DEVICE_LABEL[HR_DEVICE_SLOTS] = {
  "Bant1", "Bant2", "Bant3", "Bant4", "Bant5", "Bant6", "Bant7", "Bant8", "Bant9"
};

// ---------------- Oyuncu Roster'i (LittleFS) ----------------
// Bantlar (Polar saatler) ortak havuzdan dagitildigi icin oyuncu kimligi/
// kisisel nabiz rekoru artik fiziksel bant MAC'ine degil, buradaki roster
// kaydina baglidir (bkz. RosterStore.h). Panelden "Bant 1'i kim takiyor?"
// seciminde bir roster kaydi o slota atanir; o kaydin maxHrEver'i o slotun
// nabiz bolgesi (%HRmax) ve yorgunluk hesabinda kullanilir. Bir oyuncu farkli
// antrenmanlarda farkli fiziksel bant takabilir, rekoru hep kendi ismine
// islenmeye devam eder.
static const char* const ROSTER_FILE = "/roster.dat";
static const int MAX_ROSTER_PLAYERS = 30;  // LittleFS + RAM icin makul bir tavan
static const int ROSTER_NAME_MAX = 24;     // '\0' dahil - bkz. RosterPlayer::name

// ---------------- Nabiz tabanli Ic Yuk (Internal Load) / Yorgunluk Skoru ----------------
// 2026-08: yorgunluk skoru artik SADECE nabizdan hesaplaniyor (bkz.
// PlayerMath::calculateFatigueRisk). Internal load kavrami: Bourdon PC,
// Cardinale M, Murray A ve ark., "Monitoring Athlete Training Loads: Consensus
// Statement", Int J Sports Physiol Perform, 2017. Banister TRIMP / Edwards
// zon-agirlikli TRIMP yontemlerinden esinlenildi ama basitlestirildi: gercek
// fizyolojik HRmax olcumu YOK (cihazda yas/VO2 girisi yok) - onun yerine
// sporcunun kendi SEZON REKORU nabzi (maxHrEver) "kisisel tavan" olarak
// kullanilir. Esigin (HR_FATIGUE_BASELINE_PCT) ALTINDA katki SIFIR - hafif
// eforu (isinma/yuruyus) yorgunluk skoruna eklememek icin.
// HR_LOAD_DIVISOR muhendislik tahminidir - sahada gercek veriyle ayarlanmalidir.
// KALIBRASYON NOTU (2026-07, kullanici saha testinde bulundu): ilk deger (100)
// ~90 saniyelik izole bir efor patlamasinda bile skoru 25'e sicratiyordu - zaman
// olcegi (tum bir 30-60 dakikalik seansa yayilmasi gerekirken) TUTARSIZDI. 3000'e
// cikarilmisti.
// GUNCELLEME (2026-08): yorgunluk skoru artik SADECE nabizdan hesaplaniyor -
// onceden nabiz katkisi toplam skorun sadece bir KISMIYDI (60 dk, ort. %25
// asirilikta ~30/100 puan), simdi TEK basina 0-100 skalasini kapsamasi
// gerekiyor. Divisor 3000 -> 1000 dusuruldu: ayni referans senaryo (60 dk,
// ort. %25 asirilik = nabzin buyuk kisminda kisisel maksimuma yakin, ~%85
// civarinda seyretmesi) artik ~90 puana (KRITIK) ulasir. Bu da bir muhendislik
// tahmini - sahada gercek veriyle yeniden ayarlanmali.
static const float HR_FATIGUE_BASELINE_PCT = 60.0f;  // bu yuzdenin ALTINDA katki 0
static const float HR_LOAD_DIVISOR = 1000.0f;        // birikmis "asiri yuzde-saniye"yi skor olcegine indirger
static const float PERSONAL_MIN_HR_SAMPLE = 90.0f;   // bpm - bunun altinda sezon rekoru anlamli sayilmaz

// ---------------- Nabiz Bolgeleri (5-bolge %HRmax modeli) ----------------
// Standart spor bilimi 5-bolge modeli (Z1 cok hafif .. Z5 maksimal efor),
// kisisel sezon rekoru nabza (maxHrEver) gore yuzdelik esiklerle tanimlanir -
// tipki HR_FATIGUE_BASELINE_PCT gibi. Esikler literaturde yaygin kullanilan
// yuvarlak degerlerdir (Karvonen/%HRmax tabanli genel spor bilimi modeli),
// sahada gercek veriyle ince ayar gerekebilir.
static const float HR_ZONE1_MIN_PCT = 50.0f;  // cok hafif / isinma
static const float HR_ZONE2_MIN_PCT = 60.0f;  // hafif / yag yakimi
static const float HR_ZONE3_MIN_PCT = 70.0f;  // orta / aerobik gelisim
static const float HR_ZONE4_MIN_PCT = 80.0f;  // esik / anaerobik
static const float HR_ZONE5_MIN_PCT = 90.0f;  // maksimal efor

// ---------------- ACWR (Akut:Kronik Is Yuku Orani) / Monotonluk - Roster ----------------
// Kaynak: Williams S ve ark., Br J Sports Med 2016; Murray NB, Gabbett TJ ve
// ark., Br J Sports Med 2017 (ACWR-EWMA). Antrenman Monotonlugu/Zorlanmasi:
// Foster C ve ark., J Strength Cond Res 2001. lambda_akut = 2/(7+1),
// lambda_kronik = 2/(28+1). Bu, eski (GPS'li) TOAPERFORM_BLE_TEST surumunde
// zaten calisan/PC'de test edilmis bir hesap - buraya (roster'a, HER OYUNCU
// icin AYRI) tasindi (bkz. PlayerMath::calculateAcwr, RosterStore::acwrForPlayer).
//
// ESP32'de gercek zaman (RTC/NTP) YOK - cihaz internete cikmiyor. "Bugun"un
// gun-indeksi (epoch saniye / 86400) telefonun tarayicisindan periyodik olarak
// gelen 'ts' parametresinden (bkz. WebRoutes.cpp) tahmin edilir - kesin degil
// ama antrenman gunlerini ayirt etmek icin yeterli hassasiyette.
static const float ACWR_LAMBDA_ACUTE = 2.0f / (7.0f + 1.0f);
static const float ACWR_LAMBDA_CHRONIC = 2.0f / (28.0f + 1.0f);
static const long ACWR_MAX_LOOKBACK_DAYS = 42;  // 28g kronik pencere + isinma payi
static const char* const LOADS_FILE = "/loads.dat";  // her oyuncunun tarihli gunluk yuku (CSV: playerId,dayIndex,load)
static const int MAX_LOAD_ENTRIES_PER_PLAYER = 50;   // ACWR_MAX_LOOKBACK_DAYS + pay - okurken RAM'de tutulan tavan

// ---------------- Mac Gunu Yuk Agirligi ----------------
// Spor biliminde mac gunu yuku, ayni suredeki bir antrenmandan daha agir
// sayilir (rekabet stresi, karsi takim baskisi, mac-ici psikolojik yuk) -
// bkz. Bourdon ve ark. 2017 (Config.h "Nabiz tabanli Ic Yuk" notuyla ayni
// kaynak). Panelden "Antrenmani Sifirla" ile seans bitirilirken "Mac" olarak
// isaretlenirse, o gunun ACWR'ye giren GUNLUK yukune bu carpan uygulanir.
// Muhendislik tahmini (literaturde 1.2-2.0 araligi kullanilir) - sahada
// gercek veriyle ayarlanmali.
static const float MATCH_LOAD_MULTIPLIER = 1.5f;

// ---------------- Gunluk Wellness Anketi (Hooper Index tarzi) ----------------
// Kaynak: Hooper SL, Mackinnon LT ve ark. - basit 5 soruluk subjektif gunluk
// hazirlik anketi, spor bilimi literaturunde yaygin kullanilir. HER SORU 1
// (en iyi/hic) - 10 (en kotu/cok) araliginda, AYNI YONDE (dusuk = iyi) - bu
// yuzden 5 sorunun toplami dogrudan bir "wellness skoru" olusturur (5=en iyi,
// 50=en kotu). Bantlar (nabiz) ile ayni platformda ama TAMAMEN BAGIMSIZ bir
// veri kaynagi - antrenman ONCESI, elle, panelden girilir.
// ESIKLER (5-50 arasi toplam skor) muhendislik tahminidir, klinik esik degil -
// sahada gercek veriyle ayarlanmali.
static const char* const WELLNESS_FILE = "/wellness.dat";  // CSV: playerId,dayIndex,sleep,fatigue,soreness,stress,mood
static const int WELLNESS_GOOD_MAX = 20;    // bu toplamin ALTINDA/ESITI "iyi"
static const int WELLNESS_MODERATE_MAX = 32;  // bu toplamin ALTINDA/ESITI "orta", ustu "kotu"

// ---------------- HRV Taban Cizgisi (Readiness Baseline) ----------------
// 2026-08 (ekleme): PlayerMath::calculateHrv zaten ANLIK/oturum-ici RMSSD
// hesapliyordu (bkz. yukaridaki "RR-Interval / HRV" notu) - burada eklenen,
// o oturum ORTALAMASININ oyuncunun KENDI GECMISINE gore tipik seviyesini
// (EWMA ile) takip eden bir taban cizgisi. Hicbir tahmini/varsayimsal deger
// YOK - sadece GERCEKTEN olculen RR-interval'lardan otomatik turetilir, hicbir
// manuel giris/olcum adimi gerekmez (bkz. RosterStore.cpp - bir ara TRIMP icin
// manuel bir restingHr alani denenmis ve kullanici talebiyle geri alinmisti;
// buradaki taban tamamen otomatik oldugu ve "kalp ne diyorsa" onun disina
// cikmadigi icin o kaygi burada gecerli degil). ONEMLI SINIR: hala EGZERSIZ-ICI
// HRV'dir (dinlenik/toparlanma HRV'si degil, bkz. metodolojik sinir notu) -
// bu deger sadece oyuncunun KENDI tipik oturum-ici RMSSD'sinden bugunku
// SAPMAYI gosterir, klinik bir "toparlanma skoru" degildir.
static const float HRV_BASELINE_LAMBDA = 0.3f;       // EWMA agirligi: yeni oturumun tabana katkisi
static const int HRV_BASELINE_MIN_SESSIONS = 3;      // bu kadar oturumdan once taban "hazir" sayilmaz

// ---------------- Composite Hazir Olma Skoru (Readiness) ----------------
// Wellness + ACWR + HRV taban sapmasini TEK bir 0-100 skora birlestirir - koc
// antrenman ONCESI (sabah) tek bakista "bu oyuncuyu bugun ne kadar yukleyeyim"
// sorusuna kaba bir cevap alsin diye (bkz. PlayerMath::calculateReadiness).
// Muhendislik tahmini agirliklandirma - klinik/bilimsel olarak DOGRULANMAMIS,
// sahada gercek veriyle ayarlanmali. Eksik veriden uydurma skor URETILMEZ -
// en az 2 bilesen mevcut degilse "Yetersiz veri" doner.
static const int READINESS_GOOD_MIN = 70;      // bu ve uzeri "HAZIR"
static const int READINESS_MODERATE_MIN = 45;  // bu ve uzeri (70'in altinda) "ORTA", altı "DIKKAT"

// ---------------- Oyuncu Bazli Cok-Oturumlu Trend ----------------
// 2026-08 (ekleme): SeasonStore'daki "Bant 1" gecmisinden (history.jsonl)
// BAGIMSIZ - roster'a atanmis HER oyuncu icin, her TAMAMLANMIS oturumun
// ozetini (yorgunluk skoru + varsa HRV taban sapmasi) ayri bir dosyaya
// ekler. Focus Modu'nda "son N oturum" trend grafigi icin (bkz.
// RosterStore::recordSessionLog/sessionLogForPlayer).
static const char* const SESSION_LOG_FILE = "/sessionlog.dat";  // CSV: playerId,dayIndex,fatigueScore,hrvDeviationPct
static const int MAX_SESSION_LOG_ENTRIES_PER_PLAYER = 20;  // trend grafiginde gosterilen son oturum sayisi
static const long SESSION_LOG_MAX_LOOKBACK_DAYS = 120;      // dosya budama siniri (LOADS_FILE ile ayni desen)
