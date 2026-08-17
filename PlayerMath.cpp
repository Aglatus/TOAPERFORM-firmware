#include "PlayerMath.h"

namespace PlayerMath {

RiskResult calculateFatigueRisk(const FatigueInputs& in) {
  float score = in.hrLoadAccum / HR_LOAD_DIVISOR;

  if (score < 0) score = 0;
  if (score > 100) score = 100;

  RiskResult r;
  r.score = (int)(score + 0.5f);

  if (r.score <= 25) {
    r.status = "NORMAL";
    r.colorHex = "#22c55e";
  } else if (r.score <= 40) {
    r.status = "UYARI";
    r.colorHex = "#facc15";
  } else {
    r.status = "KRITIK";
    r.colorHex = "#ef4444";
  }

  return r;
}

void FatigueTrendTracker::reset() {
  index_ = 0;
  count_ = 0;
  trendPerMin_ = 0;
  etaMinutes_ = -1;
}

void FatigueTrendTracker::addSample(int fatigueScore) {
  int oldestValue;
  if (count_ < FATIGUE_TREND_WINDOW) {
    oldestValue = buffer_[0];
  } else {
    oldestValue = buffer_[index_];
  }

  buffer_[index_] = fatigueScore;
  index_ = (index_ + 1) % FATIGUE_TREND_WINDOW;
  if (count_ < FATIGUE_TREND_WINDOW) count_++;

  if (count_ < FATIGUE_TREND_MIN_SAMPLES) {
    trendPerMin_ = 0;
    etaMinutes_ = -1;
    return;
  }

  float elapsedMin = count_ / 60.0f;
  trendPerMin_ = (fatigueScore - oldestValue) / elapsedMin;

  if (trendPerMin_ > 0.5f && fatigueScore < 40) {
    float minutesToCritical = (40 - fatigueScore) / trendPerMin_;
    etaMinutes_ = (minutesToCritical <= 60) ? (int)(minutesToCritical + 0.5f) : -1;
  } else {
    etaMinutes_ = -1;
  }
}

int hrZoneFromPercent(float pctOfPersonalMax) {
  if (pctOfPersonalMax >= HR_ZONE5_MIN_PCT) return 5;
  if (pctOfPersonalMax >= HR_ZONE4_MIN_PCT) return 4;
  if (pctOfPersonalMax >= HR_ZONE3_MIN_PCT) return 3;
  if (pctOfPersonalMax >= HR_ZONE2_MIN_PCT) return 2;
  if (pctOfPersonalMax >= HR_ZONE1_MIN_PCT) return 1;
  return 0;
}

}  // namespace PlayerMath
