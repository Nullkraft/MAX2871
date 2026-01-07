#include <Arduino.h>
#include "max2871.h"
#include "smoke_hal.h"

void setup() {
    SmokeHAL hal(0);
    MAX2871 lo(66.0, hal);
    (void) lo;
}

void loop() {}
