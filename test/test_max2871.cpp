#ifdef ARDUINO
  #include <Arduino.h>
#endif
#include "max2871.h"
#include <unity.h>
#include "mock_hal.h"

// Shared test object
MAX2871 lo(66.0);  // Reference clock = 66 MHz
float tolerance = 0.002;        // +/- 1 kHz

// --- Unity Test Fixtures ---
void setUp(void) {
    // Baseline frequency for the member-variable tests
    lo.freq2FMN(4129.392);  
#ifdef ARDUINO
    Serial.begin(115200);
#endif
}

void tearDown(void) {}



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
    lo_if->begin(17);

    lo_if->setFrequency(4192.392);          // exercise interface to setFrequency()
    TEST_ASSERT_FALSE(lo_if->isLocked());   // MockHAL returns false
}

// Test Set All Registers
void test_setAllRegisters_writes_all_registers_in_order(void) {
    MockHAL mock;
    MAX2871 lo;
    lo.attachHal(&mock);

    // Fill shadow registers with known dummy values...
    for (int reg = 0; reg <= 6; ++reg) {
        lo.Curr.Reg[reg] = 0xAAAA0000 | reg;    // reg is the register address (3 lsb)
    }

    lo.setAllRegisters();
    // Assert: Expecting exactly 7 writes
    TEST_ASSERT_EQUAL_UINT8(7, mock.writeCount);

    // Expect values to be written in reverse order, R6 --> R0
    for (int regAddr = 6; regAddr >= 0; --regAddr) {
        uint32_t expectedVal = 0xAAAA0000 | regAddr;
        TEST_ASSERT_EQUAL_HEX32(expectedVal, regAddr);
    }

    // Clear the shadow register dirty mask
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void runAllTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_known);
    RUN_TEST(test_lowest_freq);
    RUN_TEST(test_highest_freq);
    RUN_TEST(test_integerN_case);
    RUN_TEST(test_param_round_trip);
    RUN_TEST(test_interface_begin_and_setFrequency);
    UNITY_END();
}

// --- Entry Point for Both Runners ---
#ifdef ARDUINO
void setup() {
    Serial.begin(115200);
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
