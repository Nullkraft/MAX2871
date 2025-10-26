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
static BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE, RF_EN);
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

// --- Round-trip Test for Known Case ---
void test_round_trip_known(void) {
    float tolerance = 0.002;        // +/- 1 kHz
    double freq = 75.00;
    lo.outputPower(+5, RF_ALL);
    lo.outputSelect(RF_ALL);
    lo.setFrequency(freq);  // Set the frequency by calculating Frac, M, and N
    lo.freq2FMN(freq);      // Get calculated frequency from on Frac, M, and N
    TEST_ASSERT_FLOAT_WITHIN(tolerance, freq, lo.fmn2freq());
}

void test_set_RF_output_power_level(void) {
    uint8_t mask = 0xD8;
    lo.outputPower(-4, RF_ALL);
    TEST_ASSERT_BITS_MESSAGE(mask, 0x0, lo.Curr.Reg[4], "*** -4 dBm ***");
    lo.outputPower(-1, RF_ALL);
    TEST_ASSERT_BITS_MESSAGE(mask, 0x48, lo.Curr.Reg[4], "*** -2 dBm ***");
    lo.outputPower(+2, RF_ALL);
    TEST_ASSERT_BITS_MESSAGE(mask, 0x90, lo.Curr.Reg[4], "*** +1 dBm ***");
    lo.outputPower(+5, RF_ALL);
    TEST_ASSERT_BITS_MESSAGE(mask, 0xD8, lo.Curr.Reg[4], "*** +5 dBm ***");
}

// To see the printed output run 'pio test -ve eval_board'
void test_default_setup() {
    TEST_MESSAGE("*** Registers ***");
    print_registers(lo);
}

int runUnityTests() {
    UNITY_BEGIN();
    RUN_TEST(test_default_setup);
    RUN_TEST(test_round_trip_known);
    RUN_TEST(test_set_RF_output_power_level);
    return UNITY_END();
}

void setup() {
    delay(2000);     // give serial monitor time to connect
    hal.begin();
    lo.attachHal(&hal);
    lo.begin(PIN_LE);
    hal.setCEPin(true);     // <--- This sets RF Enable Pin5 High
    runUnityTests();
}

void loop() {
    lo.reset();
    delay(500);
}

#endif // ARDUINO
// #endif // if not ARDUINO
