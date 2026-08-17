#pragma once
// =====================================================
// TOAPERFORM - TeamNetwork.h
// Donanim adaptoru: ESP-NOW ile ayni WiFi kanalindaki diger TOAPERFORM
// cihazlarina kompakt bir nabiz/yorgunluk ozeti yayinlar ve (HUB isaretliyse)
// digerlerinden gelen yayinlari dinleyip bir "peer tablosu"nda tutar.
//
// 2026-08: kapsam SADECE nabza daraltildigi icin ozet artik sadece bpm/
// yorgunluk/risk/nabiz bolgesi tasir - eski surumdeki hiz/mesafe/sprint/
// sicrama/rating alanlari (GPS/Kariyer Karti ile birlikte) kaldirildi.
// Ayni sebeple carpisma tespiti (GPS/sert-yavaslama tabanliydi) de YOK.
// =====================================================
#include <esp_now.h>
#include <WiFi.h>
#include "Config.h"

struct PeerSummary {
  char deviceName[16] = "";
  int heartRateBpm = 0;
  int fatigueScore = 0;
  char riskStatus[16] = "NORMAL";
  int hrZone = 0;              // 0 (esik alti) veya 1-5 (Z1..Z5)
  unsigned long sessionSeconds = 0;
};

struct PeerRecord {
  PeerSummary data;
  unsigned long lastSeenMs = 0;
  bool used = false;
};

class TeamNetwork {
public:
  void begin();  // esp_now_init + broadcast peer ekleme

  void broadcast(const PeerSummary& selfSummary);

  // PEER_TIMEOUT_MS'den uzun suredir yayin gelmeyen kayitlari "used=false"
  // yapar - loop() icinde periyodik cagrilmalidir (bkz. handleTeam).
  void expireStalePeers(unsigned long nowMs);

  const PeerRecord* peerTable() const { return peerTable_; }
  int maxPeers() const { return MAX_PEERS; }

private:
  static TeamNetwork* instance_;
  PeerRecord peerTable_[MAX_PEERS];

  static void onRecvStatic(const uint8_t* mac, const uint8_t* incomingData, int len);
  void onRecv(const uint8_t* incomingData, int len);
};
