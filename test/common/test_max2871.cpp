#include "max2871.h"
#include <unity.h>

MAX2871_LO lo(66.0);

void setUp(void) {
    lo.freq2FMN(4129.392);
}

void tearDown(void) {}

void test_round_trip_known(void) {
    double freq = 4129.392;
    lo.freq2FMN(freq);
    TEST_ASSERT_FLOAT_WITHIN(0.001, freq, lo.fmn2freq());
}

void test_integerN_case(void) {
    double freq = 2970.0;
    lo.freq2FMN(freq);
    TEST_ASSERT_EQUAL_UINT32(0, lo.Frac);
    TEST_ASSERT_FLOAT_WITHIN(0.001, freq, lo.fmn2freq());
}

// Entry point that runners will call
void runAllTests() {
    RUN_TEST(test_round_trip_known);
    RUN_TEST(test_integerN_case);
}
 
