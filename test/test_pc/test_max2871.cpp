#ifdef ARDUINO
  #include <Arduino.h>
#endif
#include "max2871.h"
#include <unity.h>
#include "mock_hal.h"
#include <stdio.h>

// Shared test object
MockHAL hal;
MAX2871 lo(66000, hal, hal);  // Reference clock = 66 MHz
uint32_t tolerance = 2;       // +/- 2 kHz

// --- Unity Test Fixtures ---
void setUp(void) {
    // Baseline frequency for the member-variable tests
    lo.freq2FMN(4129392);
#ifdef ARDUINO
    Serial.begin(115200);
#endif
}

void tearDown(void) {}

// --- Round-trip Test for Known Case ---
void test_round_trip_known(void) {
    uint32_t freq = 4129392;
    lo.freq2FMN(freq);
    TEST_ASSERT_UINT32_WITHIN(tolerance, freq, lo.fmn2freq());
}

// --- Boundary Tests ---
void test_lowest_freq(void) {
    uint32_t freq = 23500;   // min device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_UINT32_WITHIN(tolerance, freq, lo.fmn2freq());
}

void test_highest_freq(void) {
    uint32_t freq = 6000000; // max device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_UINT32_WITHIN(tolerance, freq, lo.fmn2freq());
}

void test_out_of_range_freq(void) {
    const uint32_t invalidFrequencies[] = {0, 23499, 6000001, 6001001};
    for (auto freq : invalidFrequencies) {
        lo.freq2FMN(4129392);
        const int r = lo.R;
        const MAX2871::max2871Registers registers = lo.Curr;

        lo.setFrequency(freq);

        TEST_ASSERT_EQUAL_INT(r, lo.R);
        TEST_ASSERT_EQUAL_UINT32(0, lo.Fpfd);
        TEST_ASSERT_EQUAL_UINT16(0, lo.Frac);
        TEST_ASSERT_EQUAL_UINT16(0, lo.M);
        TEST_ASSERT_EQUAL_UINT16(0, lo.N);
        TEST_ASSERT_EQUAL_UINT8(0, lo.DIVA);
        TEST_ASSERT_EQUAL_UINT32(0, lo.fmn2freq());
        TEST_ASSERT_EQUAL_UINT32_ARRAY(registers.Reg, lo.Curr.Reg,
                                       MAX2871::max2871Registers::numRegisters);
    }
}

// --- Integer-N Edge Case ---
void test_integerN_case(void) {
    uint32_t freq = 2970000;   // 90 * 33 MHz = integer-N
    lo.freq2FMN(freq);
    TEST_ASSERT_EQUAL_UINT32(0, lo.Frac);  // Frac = 0 in integer-N
    TEST_ASSERT_UINT32_WITHIN(tolerance, freq, lo.fmn2freq());
}

// --- Parameterized Round-trip Tests ---
struct FreqTest {
    uint32_t freq;
};

FreqTest freq_cases[] = {
    {100000},
    {915000},
    {1420000},   // hydrogen line :)
    {2400000},
    {3600000},
    {5800000}
};

void test_param_round_trip(void) {
    for (auto &tc : freq_cases) {
        lo.freq2FMN(tc.freq);
        TEST_ASSERT_UINT32_WITHIN(tolerance, tc.freq, lo.fmn2freq());
    }
}

// Interface Test
void test_interface_begin_and_setFrequency(void) {
    MockHAL hal;
    I_PLLSynthesizer* lo_if = new MAX2871(66000, hal, hal);
    lo_if->begin();

    lo_if->setFrequency(4192392);           // exercise interface to setFrequency()
    TEST_ASSERT_FALSE(lo_if->isLocked());   // MockHAL returns false
}

void test_outputSelect_marks_R4_only_and_sets_expected_bits(void) {
    /* Before the test calls outputSelect()...
     * 1) By default both outputs are enabled ==> outputSelect(RF_ALL)
     */
    uint32_t reg4_RF_AB_mask = 0x120;
    MAX2871 lo(66000, hal, hal);
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

void runAllTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_known);
    RUN_TEST(test_lowest_freq);
    RUN_TEST(test_highest_freq);
    RUN_TEST(test_out_of_range_freq);
    RUN_TEST(test_integerN_case);
    RUN_TEST(test_param_round_trip);
    RUN_TEST(test_interface_begin_and_setFrequency);
    RUN_TEST(test_outputSelect_marks_R4_only_and_sets_expected_bits);
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
