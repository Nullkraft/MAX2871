#include <Arduino.h>
#include "max2871.h"
#include "smoke_hal.h"    // from your src/

void setup() {
    MAX2871 lo(66.0);
    (void) lo;
}

void loop() {}
