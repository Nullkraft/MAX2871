#include <Arduino.h>
#include "max2871.h"
#include "arduino_hal.h"

// // Arduino Uno pinout
// static const uint8_t PIN_ATTEN   = A5;
// static const uint8_t PIN_LE_LO1  = A3;
// static const uint8_t PIN_LE_LO2  =  3;
// static const uint8_t PIN_LE_LO3  = A4;
// static const uint8_t PIN_REF_EN1 =  8;
// static const uint8_t PIN_REF_EN2 =  9;

// Metro Mini pinout
static const uint8_t PIN_ATTEN   = A5;
static const uint8_t PIN_LE_LO1  = A3;
static const uint8_t PIN_LE_LO2  =  4;
static const uint8_t PIN_LE_LO3  = A4;
static const uint8_t PIN_REF_EN1 =  5;
static const uint8_t PIN_REF_EN2 =  6;

// // Feather RP2040 pinout
// static const uint8_t PIN_ATTEN   = 25;  // GP25
// static const uint8_t PIN_LE_LO1  = 29;  // GP29
// static const uint8_t PIN_LE_LO2  =  9;  // GP09
// static const uint8_t PIN_LE_LO3  = 24;  // GP24
// static const uint8_t PIN_REF_EN1 = 10;  // GP10
// static const uint8_t PIN_REF_EN2 = 11;  // GP11

static const double  REF_MHZ    = 66.0;

MAX2871    lo1(REF_MHZ);
ArduinoHAL hal_lo1(PIN_LE_LO1);     // Attach the latch pin, IO pin 3, to LO2 LE
MAX2871    lo2(REF_MHZ);
ArduinoHAL hal_lo2(PIN_LE_LO2);     // Attach the latch pin, IO pin 3, to LO2 LE
MAX2871    lo3(REF_MHZ);
ArduinoHAL hal_lo3(PIN_LE_LO3);     // Attach the latch pin, IO pin 3, to LO2 LE

enum class LOInjectionMode : uint8_t { Low, High };

class FrequencyCalculator {
  public:
    uint8_t R = 1;
    double IF1;        // Corrected based on LO1 integer step size
    double IF1_center = 3600.0;
    double IF2 = 315.0;
    double IF3 = 45.0;
    double RefClock1 = 66.000;
    double RefClock2 = 66.666;
    LOInjectionMode LO1InjectionMode;
    LOInjectionMode LO2InjectionMode;
    LOInjectionMode LO3InjectionMode;

  // private:
    // HiMode injection defaults for LO2 and LO3
    double FreqRFin = 0.0;
    double FreqLO1  = 0.0;
    double FreqLO2  = 0.0;
    double FreqLO3  = 0.0;

  public:
    void set_LO_frequencies(double rfin, double fref, int R);
};

void FrequencyCalculator::set_LO_frequencies(double rfin, double RefClock, int R) {
  LO2InjectionMode = LOInjectionMode::High;   // TODO: Refactor so this gets set during calibration (spur mitigation)
  LO3InjectionMode = LOInjectionMode::High;
  double threshold = (RefClock == RefClock1) ? 2343.0001 : 2403.2731;
  double fpfd = RefClock / R;
  double IF1_step = fpfd * round(IF1_center / fpfd);
  LO1InjectionMode = (rfin < threshold) ? LOInjectionMode::High : LOInjectionMode::Low;
  int sign = (rfin < threshold) ? 1 : -1;
  
  FreqLO1 = fpfd * round((IF1_step + sign * rfin) / fpfd);
  lo1.setFrequency(FreqLO1);
  
  IF1 = FreqLO1 - (sign * rfin);  // IF1 is the corrected value
  FreqLO2 = (LO2InjectionMode==LOInjectionMode::High) ? IF1 + IF2 : IF1 - IF2;
  lo2.setFrequency(FreqLO2);

  FreqLO3 = (LO3InjectionMode==LOInjectionMode::High) ? IF2 + IF3 : IF2 - IF3;
  lo3.setFrequency(FreqLO3);
}

void setup() {
  delay(5000);
  Serial.begin(115200);
  SPI.begin();
  pinMode(PIN_LE_LO1, OUTPUT);
  pinMode(PIN_LE_LO2, OUTPUT);
  pinMode(PIN_LE_LO3, OUTPUT);
  pinMode(PIN_ATTEN, OUTPUT);
  pinMode(PIN_REF_EN1, OUTPUT);
  pinMode(PIN_REF_EN2, OUTPUT);
  digitalWrite(PIN_LE_LO1, LOW);
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

FrequencyCalculator fc;
uint32_t delta = 0;
void loop() {
  if (Serial.available()) {
    float mhz = Serial.parseFloat();   // 0.0 on timeout/non-number
    if (mhz > 3000.0) {
      Serial.println("Out of range. Valid: DC to 3000 MHz.");
    }
    else {
      fc.set_LO_frequencies(mhz, fc.RefClock1, 1);    // Set LO1, LO2, and LO3
      // lo2.outputPower(+5, RF_ALL);

      Serial.print("\nIF1 = ");
      Serial.print(fc.IF1, 3);
      Serial.println(" MHz\n");

      Serial.println("    | Freq MHz |  M   |  F   | N");
      Serial.println("----+----------+------+------+----");

      Serial.print("LO1 | ");
      Serial.print(fc.FreqLO1, 3);
      Serial.print(" | ");
      Serial.print(lo1.M);
      Serial.print(" |    ");
      Serial.print(lo1.Frac);
      Serial.print(" | ");
      Serial.println(lo1.N);

      Serial.print("LO2 | ");
      Serial.print(fc.FreqLO2, 3);
      Serial.print(" | ");
      Serial.print(lo2.M);
      Serial.print(" | ");
      Serial.print(lo2.Frac);
      Serial.print(" | ");
      Serial.println(lo2.N);

      Serial.print("LO3 |  ");
      Serial.print(fc.FreqLO3, 3);
      Serial.print(" | ");
      Serial.print(lo3.M);
      Serial.print(" | ");
      Serial.print(lo3.Frac);
      Serial.print(" | ");
      Serial.println(lo3.N);
    }
    // Clear serial buffer after every use
    while (Serial.available()) Serial.read();
  }

  digitalWrite(PIN_LE_LO2, !digitalRead(PIN_LE_LO2));
}
