#include <Arduino.h>
#include "max2871.h"
#include "arduino_hal.h"    // from your src/

uint8_t PIN_LE = 3;
MAX2871 lo(66.0);
ArduinoHAL hal(PIN_LE);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // hal.begin(16000000UL);
    // lo.attachHal(&hal);
    // lo.begin(PIN_LE);
    // hal.pinMode(LED_BUILTIN, PINMODE_OUTPUT);
    // lo.setFrequency(50.0);
    // lo.outputSelect(RF_ALL);
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
}
