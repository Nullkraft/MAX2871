#include "frequency_calculator.h"
#include <math.h>   // round()

void FrequencyCalculator::set_LO_frequencies(double rfin, double RefClock, int R_in) {
  FreqRFin = rfin;
  R = (uint8_t)R_in;

  // TODO: set during calibration (spur mitigation)
  LO2InjectionMode = LOInjectionMode::High;
  LO3InjectionMode = LOInjectionMode::High;

  // NOTE: comparing doubles for equality is fragile; this keeps your current behavior.
  double threshold = (RefClock == RefClock1) ? 2343.0001 : 2403.2731;

  double fpfd = RefClock / R;
  double IF1_step = fpfd * round(IF1_center / fpfd);

  bool hiLo1 = (rfin < threshold);
  LO1InjectionMode = hiLo1 ? LOInjectionMode::High : LOInjectionMode::Low;
  int sign = hiLo1 ? 1 : -1;

  FreqLO1 = fpfd * round((IF1_step + sign * rfin) / fpfd);
  _lo1.setFrequency(FreqLO1);

  IF1 = FreqLO1 - (sign * rfin);

  FreqLO2 = (LO2InjectionMode == LOInjectionMode::High) ? (IF1 + IF2) : (IF1 - IF2);
  _lo2.setFrequency(FreqLO2);

  FreqLO3 = (LO3InjectionMode == LOInjectionMode::High) ? (IF2 + IF3) : (IF2 - IF3);
  _lo3.setFrequency(FreqLO3);
}
