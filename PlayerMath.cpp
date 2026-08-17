#include "PlayerMath.h"
#include <math.h>

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

AcwrResult calculateAcwr(const DayLoad* entries, int entryCount, long todayIdx) {
  AcwrResult out;
  if (entryCount <= 0) return out;

  float weeklyLoads[7] = { 0, 0, 0, 0, 0, 0, 0 };
  for (int i = 0; i < entryCount; i++) {
    long diff = todayIdx - entries[i].dayIndex;
    if (diff >= 0 && diff < 7) {
      weeklyLoads[diff] += entries[i].load;
    }
  }

  float weekSum = 0;
  for (int i = 0; i < 7; i++) weekSum += weeklyLoads[i];
  float weekMean = weekSum / 7.0f;

  float variance = 0;
  for (int i = 0; i < 7; i++) {
    float diff = weeklyLoads[i] - weekMean;
    variance += diff * diff;
  }
  variance /= 7.0f;
  float weekSD = sqrtf(variance);

  out.monotony = (weekSD > 0.01f) ? (weekMean / weekSD) : 0;
  out.strain = weekSum * out.monotony;

  long minIdx = todayIdx;
  for (int i = 0; i < entryCount; i++) {
    if (entries[i].dayIndex < minIdx) minIdx = entries[i].dayIndex;
  }
  if (todayIdx - minIdx > ACWR_MAX_LOOKBACK_DAYS) minIdx = todayIdx - ACWR_MAX_LOOKBACK_DAYS;

  for (long day = minIdx; day <= todayIdx; day++) {
    float dayLoad = 0;
    bool hasSession = false;

    for (int i = 0; i < entryCount; i++) {
      if (entries[i].dayIndex == day) {
        dayLoad += entries[i].load;
        hasSession = true;
      }
    }
    if (hasSession) out.daysWithData++;

    if (day == minIdx) {
      out.ewmaAcute = dayLoad;
      out.ewmaChronic = dayLoad;
    } else {
      out.ewmaAcute = dayLoad * ACWR_LAMBDA_ACUTE + out.ewmaAcute * (1.0f - ACWR_LAMBDA_ACUTE);
      out.ewmaChronic = dayLoad * ACWR_LAMBDA_CHRONIC + out.ewmaChronic * (1.0f - ACWR_LAMBDA_CHRONIC);
    }
  }

  if (out.daysWithData >= 3 && out.ewmaChronic > 0) {
    out.acwr = out.ewmaAcute / out.ewmaChronic;

    if (out.acwr < 0.8f) out.band = "Dusuk yuk";
    else if (out.acwr <= 1.3f) out.band = "Sweet spot";
    else if (out.acwr <= 1.5f) out.band = "Orta risk";
    else out.band = "Yuksek sakatlanma riski";
  }

  if (out.daysWithData >= 3) {
    if (out.monotony < 1.5f) out.monotonyBand = "Dusuk (iyi cesitlilik)";
    else if (out.monotony <= 2.0f) out.monotonyBand = "Orta";
    else out.monotonyBand = "Yuksek (asiri antrenman riski)";
  }

  return out;
}

float calculateRmssd(const uint16_t* rrMs, int count) {
  if (count < 2) return 0;

  float sumSq = 0;
  for (int i = 1; i < count; i++) {
    float diff = (float)rrMs[i] - (float)rrMs[i - 1];
    sumSq += diff * diff;
  }
  return sqrtf(sumSq / (count - 1));
}

}  // namespace PlayerMath
