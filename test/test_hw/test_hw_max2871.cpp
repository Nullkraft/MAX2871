// #ifndef ARDUINO
#ifdef ARDUINO

#include <Arduino.h>
#include <unity.h>

#include "max2871.h"
#include "hal_arduino.h"

// ==== Wiring & config ====
static constexpr uint8_t PIN_LE  = A3;   // MAX2871 LE (latch)
static constexpr uint8_t PIN_MUX = A0;   // MAX2871 MUXOUT (read-only)
static constexpr double  REF_MHZ = 66.0; // your reference MHz

// CE is tied HIGH on your board, so pass 0xFF to ArduinoHAL so it won't touch it.
static ArduinoHAL hal(PIN_LE, 0xFF /*CE unused*/, PIN_MUX);
static MAX2871 lo(REF_MHZ);

// Unity hooks
void setUp() {}
void tearDown() {}

__attribute__((unused)) static void print_register(MAX2871 &lo, uint8_t regAddr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "reg[%d]=0x%08lX", regAddr, lo.Curr.Reg[regAddr]);
    TEST_MESSAGE(buf);
}

__attribute__((unused)) static void print_registers(MAX2871 &lo) {
    uint32_t reg;
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
    lo.outputPower(2);       // +2 dBm is a safe middle level for most scopes
}

void test_set_freq_100MHz_for_scope(void) {
    hal.begin(4000000UL);
    lo.attachHal(&hal);
    lo.begin(A3);                 // your LE = A3
    enable_outputs_for_scope();

    // Program ~0.100 GHz (adjust if your API expects Hz or MHz—your tests implied MHz)
    lo.setFrequency(100.0);

    TEST_MESSAGE("Set MAX2871 to ~100.000 MHz. Check RFOUTA/B on the scope.");
    // Give you time to measure. Increase if you want more hands-on time.
    delay(3000);
    TEST_PASS();
}

void test_set_freq_66MHz_for_scope(void) {
    // Keep SPI already begun/attached from the prior test, but safe to repeat:
    hal.begin(20000000UL);
    lo.attachHal(&hal);
    lo.resetToDefaultRegisters();
    print_registers(lo);
    // Program ~66.00 MHz
    lo.setFrequency(66.0);
    TEST_MESSAGE("*** after set frequency ***");
    print_registers(lo);
    lo.setAllRegisters();   // program the chip

    TEST_MESSAGE("Set MAX2871 to ~66.000 MHz. Check RFOUTA/B on the scope.");
    delay(3000);
    TEST_PASS();
}

int runUnityTests() {
    UNITY_BEGIN();
    // RUN_TEST(test_begin_runs_on_hardware);
    // RUN_TEST(test_observe_muxout_level);
    // RUN_TEST(test_set_freq_100MHz_for_scope);
    RUN_TEST(test_set_freq_66MHz_for_scope);
    return UNITY_END();
}

void setup() {
    delay(2000);     // give serial monitor time to connect
    runUnityTests();
}

void loop() {}

#endif // ARDUINO
// #endif // if not ARDUINO