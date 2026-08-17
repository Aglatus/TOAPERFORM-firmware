#include "AlertOutput.h"
#include <Arduino.h>

void AlertOutput::begin() {
  pinMode(ALERT_PIN, OUTPUT);
  digitalWrite(ALERT_PIN, LOW);
}

void AlertOutput::triggerPattern(int pulses) {
  if (pulsesRemaining_ > 0) return;  // zaten bir desen calisiyor, basitlik icin atla
  pulsesRemaining_ = pulses;
  pinOn_ = false;
  nextChangeMs_ = millis();
}

void AlertOutput::update() {
  if (pulsesRemaining_ <= 0) return;
  if (millis() < nextChangeMs_) return;

  if (!pinOn_) {
    digitalWrite(ALERT_PIN, HIGH);
    pinOn_ = true;
    nextChangeMs_ = millis() + ALERT_PULSE_MS;
  } else {
    digitalWrite(ALERT_PIN, LOW);
    pinOn_ = false;
    pulsesRemaining_--;
    nextChangeMs_ = millis() + ALERT_PULSE_GAP_MS;
  }
}

void AlertOutput::updateCriticalRepeat(bool isCritical) {
  if (isCritical && (lastCriticalAlertMs_ == 0 || millis() - lastCriticalAlertMs_ >= ALERT_REPEAT_MS)) {
    triggerPattern(3);
    lastCriticalAlertMs_ = millis();
  }

  if (!isCritical) {
    lastCriticalAlertMs_ = 0;  // kritik durum bitince sifirla, tekrar girilirse hemen uyarsin
  }

  update();
}
