#include <Arduino.h>
#include "max2871.h"
#include <unity.h>
#include "mock_hal.h"

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  Serial.println("Are we there yet?");
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  delay(1000);
}
