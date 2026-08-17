#include "TeamNetwork.h"
#include <string.h>

TeamNetwork* TeamNetwork::instance_ = nullptr;

// NOT: Bu callback imzasi arduino-esp32 core 2.x (ESP-IDF v4.x) icindir.
// Core 3.x'e gecilirse imza degisir (esp_now_recv_info_t* mac bilgisi tasir).
void TeamNetwork::onRecvStatic(const uint8_t* mac, const uint8_t* incomingData, int len) {
  if (instance_) instance_->onRecv(incomingData, len);
}

void TeamNetwork::onRecv(const uint8_t* incomingData, int len) {
  if (!DEVICE_IS_HUB) return;
  if (len != sizeof(PeerSummary)) return;

  PeerSummary incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));
  incoming.deviceName[sizeof(incoming.deviceName) - 1] = '\0';
  incoming.riskStatus[sizeof(incoming.riskStatus) - 1] = '\0';

  // Kendi cihazimizdan (veya ayni ismi tasiyan baska bir cihazdan) gelen
  // paketi yoksay - deviceName her cihazda benzersiz olmali.
  if (strncmp(incoming.deviceName, DEVICE_NAME, sizeof(incoming.deviceName)) == 0) return;

  int slot = -1;
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peerTable_[i].used && strncmp(peerTable_[i].data.deviceName, incoming.deviceName, sizeof(incoming.deviceName)) == 0) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    for (int i = 0; i < MAX_PEERS; i++) {
      if (!peerTable_[i].used) {
        slot = i;
        break;
      }
    }
  }
  if (slot < 0) return;  // tablo dolu - basitlik icin en eski kaydi atmiyoruz

  peerTable_[slot].data = incoming;
  peerTable_[slot].lastSeenMs = millis();
  peerTable_[slot].used = true;
}

void TeamNetwork::begin() {
  instance_ = this;

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW baslatilamadi. Takim ozelligi devre disi.");
    return;
  }

  esp_now_register_recv_cb(onRecvStatic);

  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, 6);  // yayin (broadcast) adresi
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_AP;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ESP-NOW yayin eslenemedi.");
  }
}

void TeamNetwork::broadcast(const PeerSummary& selfSummary) {
  uint8_t broadcastAddr[6];
  memset(broadcastAddr, 0xFF, 6);
  esp_now_send(broadcastAddr, (const uint8_t*)&selfSummary, sizeof(selfSummary));
}

void TeamNetwork::expireStalePeers(unsigned long nowMs) {
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peerTable_[i].used && nowMs - peerTable_[i].lastSeenMs > PEER_TIMEOUT_MS) {
      peerTable_[i].used = false;
    }
  }
}
