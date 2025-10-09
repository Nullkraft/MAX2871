#include <Arduino.h>
#include <max2871.h>     // from your src/

void setup() {
  // Minimal compile-time sanity only (no pins required to compile)
  MAX2871 lo(66.0);
  (void)lo;
}

void loop() {}
