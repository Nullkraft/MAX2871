#include <Arduino.h>
#include "max2871.h"
#include <unity.h>

MAX2871_LO lo(66.0);  // Reference clock 66 MHz

// --- Test Fixtures ---
void setUp(void) {
    Serial.begin(115200);
    // Baseline frequency for the member-variable tests
    lo.freq2FMN(4129.392);  
}

void tearDown(void) {}

// --- Baseline Tests ---
void test_M(void) {
    TEST_ASSERT_EQUAL_UINT16(4095, lo.M);
}

void test_Frac(void) {
    TEST_ASSERT_EQUAL_UINT32(2320, lo.Frac);
}

void test_N(void) {
    TEST_ASSERT_EQUAL_UINT16(62, lo.N);
}

void test_Diva(void) {
    TEST_ASSERT_TRUE(lo.DIVA < 8);
}

// --- Round-trip Test for Known Case ---
void test_round_trip_known(void) {
    double freq = 4129.392;
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(0.0005, freq, lo.fmn2freq());  // ±500 Hz
}

// --- Boundary Tests ---
void test_lowest_freq(void) {
    double freq = 23.5;   // min device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(0.0005, freq, lo.fmn2freq());
}

void test_highest_freq(void) {
    double freq = 6000.0; // max device spec
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(0.0005, freq, lo.fmn2freq());
}

// --- Integer-N Edge Case ---
void test_integerN_case(void) {
    double freq = 2970.0;   // 90 * 33 MHz = integer-N
    lo.freq2FMN(freq);
    TEST_ASSERT_EQUAL_UINT32(0, lo.Frac);  // Frac = 0 in integer-N
    TEST_ASSERT_FLOAT_WITHIN(0.0005, freq, lo.fmn2freq());
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
        TEST_ASSERT_FLOAT_WITHIN(0.0005, tc.freq, lo.fmn2freq());
    }
}

// --- Unity Boilerplate ---
void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_M);
    RUN_TEST(test_Frac);
    RUN_TEST(test_N);
    RUN_TEST(test_Diva);
    RUN_TEST(test_round_trip_known);
    RUN_TEST(test_lowest_freq);
    RUN_TEST(test_highest_freq);
    RUN_TEST(test_integerN_case);
    RUN_TEST(test_param_round_trip);
    UNITY_END();
}

void loop() {
    delay(1000);
}
