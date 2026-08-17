#pragma once
// =====================================================
// TOAPERFORM - AlertOutput.h
// Donanim adaptoru: titresim motoru/buzzer icin non-blocking (delay()
// kullanmayan) desen kontrolcusu. Sporcu oyun icindeyken telefona bakmadan
// bilgi alabilsin diye farkli olaylar farkli titresim SAYISIYLA bildirilir:
//   1 titresim = yeni kisisel/sezon hiz rekoru
//   3 titresim = KRITIK yorgunluk (tekrar eden uyari)
// =====================================================
#include "Config.h"

class AlertOutput {
public:
  void begin();  // pinMode

  void triggerPattern(int pulses);

  // loop() icinde her turda cagrilmali - deseni ilerletir.
  void update();

  // riskStatus "KRITIK" oldugu surece periyodik olarak 3 titresimlik deseni
  // tetikler. loop() icinde her turda cagrilmalidir.
  void updateCriticalRepeat(bool isCritical);

private:
  int pulsesRemaining_ = 0;
  bool pinOn_ = false;
  unsigned long nextChangeMs_ = 0;
  unsigned long lastCriticalAlertMs_ = 0;
};
