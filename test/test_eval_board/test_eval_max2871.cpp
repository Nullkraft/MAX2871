/* test_eval_max2871.cpp
    (MAX2871 DIY Evaluation board)

    Provides a platform for programming and testing the operation
    of the max2871 PLL Synthesizer chip.
 */
#ifdef ARDUINO

#include <Arduino.h>
#include <unity.h>

#include "max2871.h"
#include "bitbang_hal.h"

// ==== Wiring & config ====
static constexpr uint8_t RF_EN   =  5;      // MAX2871 Enable the RF Output
static constexpr uint8_t PIN_LE  = A3;      // MAX2871 LE (latch)
static constexpr uint8_t PIN_DAT = A2;      // MAX2871 DAT (data)
static constexpr uint8_t PIN_CLK = A1;      // MAX2871 CLK (clock)
static constexpr uint8_t PIN_MUX = A0;      // MAX2871 MUXOUT (read-only)
static constexpr double  REF_MHZ = 60.0;    // Reference Clock (MHz)

// CE is tied HIGH on the evaluation board
static BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE);
static MAX2871 lo(REF_MHZ);

// Unity hooks
void setUp() {}
void tearDown() {}

__attribute__((unused)) static void print_register(MAX2871 &lo, uint8_t regAddr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "reg[%d] = 0x%08lX", regAddr, lo.Curr.Reg[regAddr]);
    TEST_MESSAGE(buf);
}

__attribute__((unused)) static void print_registers(MAX2871 &lo) {
    for (int regAddr = 0; regAddr <= 6; ++regAddr) {
        print_register(lo, regAddr);
    }
}

__attribute__((unused)) static void print_hex(uint32_t val) {
    char buf[128];
    snprintf(buf, sizeof(buf), "0x%08lX", val);
    TEST_MESSAGE(buf);
}

void outputEnable(uint8_t rfEn) {
    pinMode(rfEn, PINMODE_OUTPUT);
    digitalWrite(rfEn, PINLEVEL_HIGH);
}

}

int runUnityTests() {
    UNITY_BEGIN();
    return UNITY_END();
}

void setup() {
    delay(2000);     // give serial monitor time to connect
    runUnityTests();
}

void loop() {}

#endif // ARDUINO
// #endif // if not ARDUINO
