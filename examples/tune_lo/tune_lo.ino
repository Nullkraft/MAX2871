#include <Arduino.h>
#include "max2871.h"
#include "arduino_hal.h"

// SPI (UNO): MOSI=11, MISO=12 (unused), SCK=13
// LO2 LE pin (adjust if different on your board):
static const uint8_t PIN_ATTEN   = A5;
static const uint8_t PIN_LE_LO1  = A3;
static const uint8_t PIN_LE_LO2  = 3;
static const uint8_t PIN_LE_LO3  = A4;
static const uint8_t PIN_REF_EN1 = 8;
static const uint8_t PIN_REF_EN2 = 9;

static const double  REF_MHZ    = 66.0;

MAX2871    lo2(REF_MHZ);
ArduinoHAL hal_lo2(PIN_LE_LO2);

// Tune helper
static void tune_lo2(double mhz) {
  lo2.setFrequency(mhz);   // adjust if your API returns/needs something different
  Serial.print("LO2 tuned to ");
  Serial.print(mhz, 6);
  Serial.println(" MHz");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LE_LO1, OUTPUT);
  pinMode(PIN_LE_LO2, OUTPUT);
  pinMode(PIN_LE_LO3, OUTPUT);
  pinMode(PIN_ATTEN, OUTPUT);
  pinMode(PIN_REF_EN1, OUTPUT);
  pinMode(PIN_REF_EN2, OUTPUT);
  digitalWrite(PIN_LE_LO3, LOW);
  digitalWrite(PIN_ATTEN, LOW);
  digitalWrite(PIN_REF_EN1, HIGH);
  digitalWrite(PIN_REF_EN2, LOW);

  lo2.attachHal(&hal_lo2);
  lo2.begin();
  lo2.outputSelect(RF_ALL);   // enable A+B
  lo2.outputPower(+5, RF_ALL);    // +5 dBm (per your mapping)

  Serial.println("Enter LO2 frequency in MHz (23.5 to 6000). Example: 2412.5");
  Serial.setTimeout(15); // makes parseFloat() snappy without long blocking

  // Optional: start tone so the scope shows something immediately
//   tune_lo2(2400.000);
}

void loop() {
  if (Serial.available()) {
    double mhz = Serial.parseFloat();   // 0.0 on timeout/non-number

    if (mhz < 23.5 || mhz > 6000.0) {
      Serial.println("Out of range. Valid: 23.5 to 6000 MHz.");
    } else {
      tune_lo2(mhz);                    // calls driver setFrequency(float)
    }

    // Clear any leftover chars (e.g., CR/LF) before next entry
    while (Serial.available()) Serial.read();
  }
}

