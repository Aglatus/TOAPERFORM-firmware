#pragma once
// =====================================================
// TOAPERFORM - WebRoutes.h
// HTTP handler'lari (glue katmani) - alt sistemleri (Globals.h) WebUi.h'deki
// sunumla birlestirir. Hicbir hesaplama mantigi burada YENIDEN YAZILMAZ,
// sadece PlayerMath/SeasonStore/HeartRateHardware/TeamNetwork cagirilir.
// =====================================================
#include <WebServer.h>

void registerWebRoutes();
