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
    // hal.begin(16000000UL);       // conservative SPI
    // lo.attachHal(&hal);
    // lo.begin(PIN_LE);           // Performs clean-clock startup I.A.W. the spec sheet
    lo.outputPower(+2, RF_ALL);
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
    lo.outputSelect(RF_ALL);      // both outputs ON
    // If your mapping is 0=-4dBm, 1=-1dBm, 2=+2dBm, 3=+5dBm (from earlier work):
    lo.outputPower(+5, RF_A);       // +2 dBm is a safe middle level for most scopes
    lo.outputPower(+5, RF_B);
}

/* Test Frequency  */
void test_verify_frequency_calculations(void) {
    pinMode(REF_EN1, OUTPUT);
    digitalWrite(REF_EN1, HIGH);
    float freq_in = 3213.579;
    lo.setFrequency(freq_in);
    float freq_fmn = lo.fmn2freq();
    lo.outputSelect(RF_ALL);
    lo.outputPower(+5, RF_A);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(freq_in, freq_fmn, "*** Verify frequency calculations ***");
}

void test_default_setup() {
    // TEST_MESSAGE("*** Registers ***");
    // print_registers(lo);
}

int runUnityTests() {
    UNITY_BEGIN();
    RUN_TEST(test_default_setup);
    // RUN_TEST(test_begin_runs_on_hardware);
    // RUN_TEST(test_observe_muxout_level);
    RUN_TEST(test_verify_frequency_calculations);
    return UNITY_END();
}

void setup() {
    // delay(2000);             // give serial monitor time to connect
    hal.begin(16000000UL);
    lo.attachHal(&hal);
    lo.begin(PIN_LE);        // Performs clean-clock startup I.A.W. the spec sheet
    runUnityTests();
    hal.pinMode(LED_BUILTIN, PINMODE_OUTPUT);
    lo.setFrequency(50.0);
    lo.outputSelect(RF_ALL);
}

void loop() {
    lo.setFrequency(30.0);
    delay(500);
    lo.setFrequency(32.0);
    delay(500);
}

#endif // ARDUINO
// #endif // if not ARDUINO
