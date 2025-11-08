#ifdef ARDUINO
  #include <Arduino.h>
#endif
#include "max2871.h"
#include <unity.h>
#include "mock_hal.h"
#include <stdio.h>

// Shared test object
MAX2871 lo(66.0);  // Reference clock = 66 MHz
float tolerance = 0.002;        // +/- 1 kHz
constexpr uint8_t NUM_REGS = MAX2871::max2871Registers::numRegisters;

// --- Unity Test Fixtures ---
void setUp(void) {
    // Baseline frequency for the member-variable tests
    lo.freq2FMN(4129.392);  
#ifdef ARDUINO
    Serial.begin(115200);
#endif
}

void tearDown(void) {}

__attribute__((unused)) static void print_register(MAX2871 &lo, uint8_t regAddr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "reg[%d]=0x%08lX", regAddr, lo.Curr.Reg[regAddr]);
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
    double freq = 4129.392;
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, freq, lo.fmn2freq());
}

// --- Boundary Tests ---
void test_lowest_freq(void) {
    double freq = 23.5;   // min device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, freq, lo.fmn2freq());
}

void test_highest_freq(void) {
    double freq = 6000.0; // max device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, freq, lo.fmn2freq());
}

// --- Integer-N Edge Case ---
void test_integerN_case(void) {
    double freq = 2970.0;   // 90 * 33 MHz = integer-N
    lo.freq2FMN(freq);
    TEST_ASSERT_EQUAL_UINT32(0, lo.Frac);  // Frac = 0 in integer-N
    TEST_ASSERT_FLOAT_WITHIN(tolerance, freq, lo.fmn2freq());
}

// --- Parameterized Round-trip Tests ---
struct FreqTest {
    double freq;
};

FreqTest freq_cases[] = {
    {100.0},
    {915.0},
    {1420.0},   // hydrogen line :)
    {2400.0},
    {3600.0},
    {5800.0}
};

void test_param_round_trip(void) {
    for (auto &tc : freq_cases) {
        lo.freq2FMN(tc.freq);
        TEST_ASSERT_FLOAT_WITHIN(tolerance, tc.freq, lo.fmn2freq());
    }
}

// Interface Test
void test_interface_begin_and_setFrequency(void) {
    MockHAL hal;
    I_PLLSynthesizer* lo_if = new MAX2871(66.0);
    lo_if->attachHal(&hal);
    lo_if->begin();

    lo_if->setFrequency(4192.392);          // exercise interface to setFrequency()
    TEST_ASSERT_FALSE(lo_if->isLocked());   // MockHAL returns false
}

void test_outputSelect_marks_R4_only_and_sets_expected_bits(void) {
    /* Before the test calls outputSelect()...
     * 1) By default both outputs are enabled ==> outputSelect(RF_ALL)
     */
    uint32_t reg4_RF_AB_mask = 0x120;
    MAX2871 lo(66e6);
    lo.reset();
    uint32_t before = lo.Curr.Reg[4] & reg4_RF_AB_mask; // Should equal RF_ALL, default
    uint32_t after;

    const RFOutPort rfPorts[] = {RFNONE, RF_A, RF_B, RF_ALL};
    for (int i = 0; i < 4; i++) {
        lo.outputSelect(rfPorts[i]);
        after = lo.Curr.Reg[4] & reg4_RF_AB_mask;
        if (i==RFNONE) TEST_ASSERT_NOT_EQUAL(before, after);
        if (i==RF_A) TEST_ASSERT_NOT_EQUAL(before, after);
        if (i==RF_B) TEST_ASSERT_NOT_EQUAL(before, after);
        if (i>=RF_ALL) TEST_ASSERT_EQUAL(0x120, after);
    }
}

void test_outputPower_marks_R4_only_and_sets_power_bits(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);
    lo.reset();

    // Default Power Level is +5dBm with binary bits 11
    uint32_t oldPower = (lo.Curr.Reg[4] >> 6) & 0x3;
    lo.outputPower(-4, RF_B);     // Valid power levels: -4, -1, +2, +5 dBm
    lo.updateRegisters();
    uint32_t newPower = (lo.Curr.Reg[4] >> 6) & 0x3;

    char msg[100];
    snprintf(msg, sizeof(msg), "Power level should have changed from %lu", oldPower);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(oldPower, newPower, msg);
}

void runAllTests(void) {
    UNITY_BEGIN();
    // RUN_TEST(test_round_trip_known);
    // RUN_TEST(test_lowest_freq);
    // RUN_TEST(test_highest_freq);
    // RUN_TEST(test_integerN_case);
    // RUN_TEST(test_param_round_trip);
    // RUN_TEST(test_interface_begin_and_setFrequency);
    // RUN_TEST(test_setAllRegisters_writes_all_registers_in_order);
    // RUN_TEST(test_updateRegisters_no_writes_if_clean);
    // RUN_TEST(test_updateRegisters_writes_only_dirty_in_descending_order);
    // RUN_TEST(test_updateRegisters_rewrites_R0_when_R4_dirty);
    // RUN_TEST(test_updateRegisters_mixed_dirty_with_R4_forces_R0);
    RUN_TEST(test_outputSelect_marks_R4_only_and_sets_expected_bits);
    // RUN_TEST(test_outputPower_marks_R4_only_and_sets_power_bits);
    UNITY_END();
}

// --- Entry Point for Both Runners ---
#ifdef ARDUINO
void setup() {
    // important: start Serial before tests so PlatformIO can read the stream
    Serial.begin(115200);
    // give uploader/host time to settle (uploader briefly owns the port)
    delay(2000);
    runAllTests();
}
void loop() { delay(1000); }
#else
int main(void) {
    runAllTests();
    return 0;
}
#endif
