#include "Globals.h"
#include <string.h>
#include <stdio.h>

SeasonStore seasonStore;
RosterStore rosterStore;
AlertOutput alertOutput;
PlayerMath::FatigueTrendTracker fatigueTrend[HR_SLOTS];
HeartRateHardware heartRateHardware;
TeamNetwork teamNetwork;

WebServer server(80);
DNSServer dnsServer;

unsigned long sessionStartMs = 0;
unsigned long sessionSeconds = 0;

int slotPlayerId[HR_SLOTS];
char slotPlayerName[HR_SLOTS][ROSTER_NAME_MAX];

int heartRateBpm[HR_SLOTS];
bool heartRateContact[HR_SLOTS];
bool heartRateSignalFresh[HR_SLOTS];

float sessionMaxHr[HR_SLOTS];
float hrLoadAccum[HR_SLOTS];
float hrPctOfMax[HR_SLOTS];
int hrZoneNow[HR_SLOTS];
unsigned long hrZoneSeconds[HR_SLOTS][5];

int fatigueScore[HR_SLOTS];
char riskStatus[HR_SLOTS][16];
char riskColor[HR_SLOTS][9];
char lastWarning[HR_SLOTS][96];
unsigned long warningUntilMs[HR_SLOTS];

float hrRmssdMs[HR_SLOTS];
float hrSdnnMs[HR_SLOTS];
float hrPnn50[HR_SLOTS];
bool hrRrSupported[HR_SLOTS];

float hrRmssdSessionSum[HR_SLOTS];
int hrRmssdSessionCount[HR_SLOTS];

float breathingRateBpm[HR_SLOTS];

bool hrrActive[HR_SLOTS];
unsigned long hrrStartMs[HR_SLOTS];
int hrrHr0[HR_SLOTS];
int hrrHr60[HR_SLOTS];
int hrrHr120[HR_SLOTS];

void startHrrTest(int slot, int currentBpm) {
  hrrActive[slot] = true;
  hrrStartMs[slot] = millis();
  hrrHr0[slot] = currentBpm;
  hrrHr60[slot] = -1;
  hrrHr120[slot] = -1;
}

bool orthoActive[HR_SLOTS];
int orthoPhase[HR_SLOTS];
unsigned long orthoPhaseStartMs[HR_SLOTS];
float orthoBpmSum[HR_SLOTS];
int orthoBpmCount[HR_SLOTS];
float orthoRmssdSum[HR_SLOTS];
int orthoRmssdCount[HR_SLOTS];
float orthoHr1[HR_SLOTS];
float orthoHr2[HR_SLOTS];
float orthoRmssdResult1[HR_SLOTS];
float orthoRmssdResult2[HR_SLOTS];

void startOrthoTest(int slot) {
  orthoActive[slot] = true;
  orthoPhase[slot] = 1;
  orthoPhaseStartMs[slot] = millis();
  orthoBpmSum[slot] = 0;
  orthoBpmCount[slot] = 0;
  orthoRmssdSum[slot] = 0;
  orthoRmssdCount[slot] = 0;
  orthoHr1[slot] = -1;
  orthoHr2[slot] = -1;
  orthoRmssdResult1[slot] = -1;
  orthoRmssdResult2[slot] = -1;
}

unsigned long lastPeerBroadcastMs = 0;

unsigned long lastKnownEpochSec = 0;
unsigned long lastKnownEpochAtMs = 0;

void updateEpochSync(unsigned long epochSec) {
  if (epochSec == 0) return;
  lastKnownEpochSec = epochSec;
  lastKnownEpochAtMs = millis();
}

long currentDayIndex() {
  if (lastKnownEpochSec == 0) return -1;
  unsigned long estimatedEpoch = lastKnownEpochSec + (millis() - lastKnownEpochAtMs) / 1000UL;
  return (long)(estimatedEpoch / 86400UL);
}

void formatTime(unsigned long sec, char* out, size_t outSize) {
  unsigned long h = sec / 3600;
  unsigned long m = (sec % 3600) / 60;
  unsigned long s = sec % 60;
  snprintf(out, outSize, "%02lu:%02lu:%02lu", h, m, s);
}

void setWarning(int slot, const char* msg, unsigned long durationMs) {
  strncpy(lastWarning[slot], msg, sizeof(lastWarning[slot]) - 1);
  lastWarning[slot][sizeof(lastWarning[slot]) - 1] = '\0';
  warningUntilMs[slot] = millis() + durationMs;
}

void resetSession() {
  sessionSeconds = 0;

  for (int i = 0; i < HR_SLOTS; i++) {
    // slotPlayerId/slotPlayerName BILEREK SIFIRLANMAZ - oyuncu atamasi
    // antrenmandan antrenmana degil, koc panelden degistirene kadar kalicidir.
    fatigueScore[i] = 0;
    strcpy(riskStatus[i], "NORMAL");
    strcpy(riskColor[i], "#22c55e");
    strcpy(lastWarning[i], "Antrenman baslatildi");
    warningUntilMs[i] = millis() + 6000;

    sessionMaxHr[i] = 0;
    hrLoadAccum[i] = 0;
    hrPctOfMax[i] = 0;
    hrZoneNow[i] = 0;
    for (int z = 0; z < 5; z++) hrZoneSeconds[i][z] = 0;

    fatigueTrend[i].reset();

    hrrActive[i] = false;
    hrrHr0[i] = 0;
    hrrHr60[i] = -1;
    hrrHr120[i] = -1;

    hrRmssdSessionSum[i] = 0;
    hrRmssdSessionCount[i] = 0;

    orthoActive[i] = false;
    orthoPhase[i] = 0;
    orthoBpmSum[i] = 0;
    orthoBpmCount[i] = 0;
    orthoRmssdSum[i] = 0;
    orthoRmssdCount[i] = 0;
    orthoHr1[i] = -1;
    orthoHr2[i] = -1;
    orthoRmssdResult1[i] = -1;
    orthoRmssdResult2[i] = -1;
  }

  sessionStartMs = millis();
}
