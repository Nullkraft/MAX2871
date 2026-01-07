#pragma once

#include <stdint.h>
#include "I_PLLSynthesizer.h"
#include <math.h>   // if you want round() visible in headers (optional)

enum class LOInjectionMode : uint8_t { Low, High };

class FrequencyCalculator {
public:
  uint8_t R = 1;
  double IF1 = 0.0;
  double IF1_center = 3600.0;
  double IF2 = 315.0;
  double IF3 = 45.0;
  double RefClock1 = 66.000;
  double RefClock2 = 66.666;

  LOInjectionMode LO1InjectionMode = LOInjectionMode::High;
  LOInjectionMode LO2InjectionMode = LOInjectionMode::High;
  LOInjectionMode LO3InjectionMode = LOInjectionMode::High;

  double FreqRFin = 0.0;
  double FreqLO1  = 0.0;
  double FreqLO2  = 0.0;
  double FreqLO3  = 0.0;

  FrequencyCalculator(I_PLLSynthesizer& lo1, I_PLLSynthesizer& lo2, I_PLLSynthesizer& lo3)
    : _lo1(lo1), _lo2(lo2), _lo3(lo3) {}

  void set_LO_frequencies(double rfin, double refClockMHz, int r_div);

private:
  I_PLLSynthesizer& _lo1;
  I_PLLSynthesizer& _lo2;
  I_PLLSynthesizer& _lo3;
};
