#include <Arduino.h>
#include "max2871.h"
#include <unity.h>

void setUp(void) {
    // set stuff up here
    Serial.begin(115200);
}

void tearDown(void) {
    // clean stuff up here
}

MAX2871_LO lo(66.0);

void test_M(void) {
    TEST_ASSERT_EQUAL_UINT16(4092, lo.M);
}

void test_Frac(void) {
    TEST_ASSERT_EQUAL_UINT32(3162, lo.Frac);
}

void test_N(void) {
    TEST_ASSERT_EQUAL_UINT16(62, lo.N);
}

void test_Diva(void) {
    TEST_ASSERT_TRUE(lo.DIVA < 8);
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    lo.freq2FMN(4143.0);

    UNITY_BEGIN();
    RUN_TEST(test_M);
    RUN_TEST(test_Frac);
    RUN_TEST(test_N);
    RUN_TEST(test_Diva);
    UNITY_END();
}

void loop()
{
    delay(1000);
} 
