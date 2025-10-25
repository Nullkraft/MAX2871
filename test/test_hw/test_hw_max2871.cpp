// #ifndef ARDUINO
#ifdef ARDUINO

#include <Arduino.h>
#include <unity.h>

#include "max2871.h"
#include "arduino_hal.h"

// ==== Wiring & config ====
static constexpr uint8_t PIN_LE  = 3;    // LO2 Latch Enable (was A3)
static constexpr uint8_t PIN_MUX = A0;   // MAX2871 MUXOUT (read-only)
static constexpr uint8_t REF_EN1 = 8;    // Reference clock select
static constexpr double  REF_MHZ = 66.0; // 66 MHz reference

// CE is tied HIGH on evaluation board
static ArduinoHAL hal(PIN_LE, 0xFF /*CE unused*/, PIN_MUX);
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

// SPI speed note: AVR UNO tops out around ~8 MHz reliably.
void test_begin_runs_on_hardware(void) {
    hal.begin(4000000UL);       // conservative SPI
    lo.attachHal(&hal);
    lo.begin(PIN_LE);           // should issue startup R5..R0 twice in your impl
    TEST_PASS_MESSAGE("begin() executed without errors on hardware.");
}

// Optional MUXOUT observation (no hard assert since R4's MUX mode may vary)
// Enable a strict lock check later once R4 is known to be Digital Lock Detect.
void test_observe_muxout_level(void) {
    delay(20); // allow anything pending to settle a bit
    bool level = hal.readMuxout();
    char buf[64];
    snprintf(buf, sizeof(buf), "MUXOUT is %s", level ? "HIGH" : "LOW");
    TEST_MESSAGE(buf);

    // If you confirm R4 config sets MUXOUT = Digital Lock Detect,
    // you can change this to an ASSERT with a timeout-based lock wait.
    TEST_PASS_MESSAGE("Observed MUXOUT (informational).");
}

// ---- Add below your existing tests in test_hw_max2871.cpp ----

// Helper to set outputs explicitly (optional but nice for scoping)
static void enable_outputs_for_scope() {
    // If your API mapping is: 0=Off,1=A,2=B,3=Both (per your earlier tests)
    lo.outputSelect(3);      // both outputs ON
    // If your mapping is 0=-4dBm, 1=-1dBm, 2=+2dBm, 3=+5dBm (from earlier work):
    lo.outputPower(+5, RF_A);       // +2 dBm is a safe middle level for most scopes
    lo.outputPower(+5, RF_B);
}

void test_set_freq_60MHz_for_scope(void) {
    pinMode(REF_EN1, OUTPUT);       // Scope channel 2
    digitalWrite(REF_EN1, HIGH);

    // Keep SPI already begun/attached from the prior test, but safe to repeat:
    hal.begin(20000000UL);
    lo.attachHal(&hal);
    lo.begin(PIN_LE);               // Scope channel 1
    // Program ~60.00 MHz
    lo.setFrequency(60.0);
    lo.outputSelect(3);
    lo.outputPower(+5, RF_A);
    TEST_MESSAGE("*** Registers ***");
    print_registers(lo);
    TEST_PASS();
}

int runUnityTests() {
    UNITY_BEGIN();
    // RUN_TEST(test_begin_runs_on_hardware);
    // RUN_TEST(test_observe_muxout_level);
    RUN_TEST(test_set_freq_60MHz_for_scope);
    return UNITY_END();
}

void setup() {
    delay(2000);     // give serial monitor time to connect
    runUnityTests();
}

void loop() {}

#endif // ARDUINO
// #endif // if not ARDUINO
