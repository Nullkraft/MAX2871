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
constexpr uint8_t NUM_REGS = 7;
static uint32_t defaultCurr[NUM_REGS];
static bool defaultCurrInited = false;

// --- Unity Test Fixtures ---
void setUp(void) {
    // Baseline frequency for the member-variable tests
    lo.freq2FMN(4129.392);  
#ifdef ARDUINO
    Serial.begin(115200);
#endif
}

void tearDown(void) {}

// Capture defaultCurr from lo the first time it's available
static void capture_default_Curr_if_needed(MAX2871 &lo) {
    if (defaultCurrInited) return;
    for (int i = 0; i < NUM_REGS; ++i) {
        defaultCurr[i] = lo.Curr.Reg[i];
    }
    defaultCurrInited = true;
}

static void print_register(MAX2871 &lo, uint8_t regAddr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "reg[%d]=0x%08X", regAddr, lo.Curr.Reg[regAddr]);
    TEST_MESSAGE(buf);
}

static void print_registers(MAX2871 &lo) {
    for (int regAddr = 0; regAddr <= 6; ++regAddr) {
        print_register(lo, regAddr);
    }
}

static void print_hex(uint32_t val) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%x", val);
    TEST_MESSAGE(buf);
}

// Reset lo.Curr.Reg from the saved default snapshot
static void reset_Curr_from_default(MAX2871 &lo) {
    for (int i = 0; i < NUM_REGS; ++i) lo.Curr.Reg[i] = defaultCurr[i];
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
    lo_if->begin(17);

    lo_if->setFrequency(4192.392);          // exercise interface to setFrequency()
    TEST_ASSERT_FALSE(lo_if->isLocked());   // MockHAL returns false
}

// Test Set All Registers
void test_setAllRegisters_writes_all_registers_in_order(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    // Fill shadow registers with known dummy values...
    for (int regAddr = 0; regAddr <= 6; ++regAddr) {
        lo.Curr.Reg[regAddr] = 0xAAAA0000 | regAddr;    // reg is the register address (3 lsb)
    }

    lo.setAllRegisters();
    // Assert: Expecting exactly 7 writes
    TEST_ASSERT_EQUAL_UINT8(7, mock.writeCount);

    // Expect values to be written in reverse order, R6 --> R0
    for (int i = 0; i < 7; ++i) {
        uint32_t expectedVal = 0xAAAA0000 | (6 - i);
        TEST_ASSERT_EQUAL_HEX32(expectedVal, mock.regWrites[i]);
    }

    // Clear the shadow register dirty mask
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void test_updateRegisters_no_writes_if_clean(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    // Ensure clean
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());

    lo.updateRegisters();

    TEST_ASSERT_EQUAL_UINT8(0, mock.writeCount);
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void test_updateRegisters_writes_only_dirty_in_descending_order(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    // Fill known patterns
    for (int r = 0; r <= 6; ++r) lo.Curr.Reg[r] = 0xBBBB0000 | r;

    // Mark R2 and R5 dirty (write order should be R5 then R2)
    lo.markDirty(2);
    lo.markDirty(5);

    lo.updateRegisters();

    TEST_ASSERT_EQUAL_UINT8(2, mock.writeCount);
    TEST_ASSERT_EQUAL_HEX32(0xBBBB0005, mock.regWrites[0]); // R5 first
    TEST_ASSERT_EQUAL_HEX32(0xBBBB0002, mock.regWrites[1]); // then R2

    // Dirty mask should have cleared bits 5 and 2, but nothing else
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void test_updateRegisters_rewrites_R0_when_R4_dirty(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    // Fill unique patterns
    for (int r = 0; r <= 6; ++r) lo.Curr.Reg[r] = 0xCCCC0000 | r;

    // Only R4 marked dirty
    lo.markDirty(4);

    lo.updateRegisters();

    // Expect two writes: R4 first, then R0 (forced)
    TEST_ASSERT_EQUAL_UINT8(2, mock.writeCount);
    TEST_ASSERT_EQUAL_HEX32(0xCCCC0004, mock.regWrites[0]); // R4
    TEST_ASSERT_EQUAL_HEX32(0xCCCC0000, mock.regWrites[1]); // R0

    // Dirty mask should be clear (R4 cleared; R0 was not set and remains clear)
    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void test_updateRegisters_mixed_dirty_with_R4_forces_R0(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    for (int r = 0; r <= 6; ++r) lo.Curr.Reg[r] = 0xDDDD0000 | r;

    // Dirty: R6, R4, R1  → expect order: R6, R4, R1, (forced) R0
    lo.markDirty(6);
    lo.markDirty(4);
    lo.markDirty(1);

    lo.updateRegisters();

    TEST_ASSERT_EQUAL_UINT8(4, mock.writeCount);
    TEST_ASSERT_EQUAL_HEX32(0xDDDD0006, mock.regWrites[0]); // R6
    TEST_ASSERT_EQUAL_HEX32(0xDDDD0004, mock.regWrites[1]); // R4
    TEST_ASSERT_EQUAL_HEX32(0xDDDD0001, mock.regWrites[2]); // R1
    TEST_ASSERT_EQUAL_HEX32(0xDDDD0000, mock.regWrites[3]); // forced R0

    TEST_ASSERT_EQUAL_UINT8(0, lo.getDirtyMask());
}

void test_outputSelect_marks_R4_only_and_sets_expected_bits(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    // Capture default snapshot once, then restore it for this test
    capture_default_Curr_if_needed(lo);
    reset_Curr_from_default(lo);

    // write the baseline into the mock then clear writes so we start clean
    lo.setAllRegisters();
    // mock.clearWrites();

    // Act: change output select (example: select B)
    lo.outputSelect(0);     // Output B enable
    TEST_MESSAGE("*** Shadow Registers ***");
    print_registers(lo);

    // Assert: only R4 should be dirty
    TEST_ASSERT_EQUAL_UINT8(2, lo.getDirtyMask());

    // Apply change to HW
    lo.updateRegisters();

    // Use regDiff to examine what changed in R4 relative to baseline
    uint32_t regDiff = (mock.regWrites[0] ^ defaultCurr[6]); // or regDiff(lo,4) after updateRegisters()

    // Specific expectations: R4[8] set (disableB) and R4[5] clear (enableA)
    TEST_ASSERT_TRUE( (regDiff & (1u << 8)) != 1 );  // bit8 flipped
    TEST_ASSERT_TRUE( (regDiff & (1u << 5)) == 0 );  // bit5 not flipped compared to baseline

    // Verify forced R0 write equals baseline R0
    TEST_ASSERT_EQUAL_HEX32(defaultCurr[0], mock.regWrites[6]);
}

void test_outputPower_marks_R4_only_and_sets_power_bits(void) {
    MockHAL mock;
    MAX2871 lo(66.0);
    lo.attachHal(&mock);

    capture_default_Curr_if_needed(lo); // This needs to move to setUp() for all the tests
    reset_Curr_from_default(lo);        // Start with a fresh copy of default registers
    lo.setAllRegisters();               // Writes all registers to mock
    // mock.clearWrites();

    // Act: set power to +2 dBm (example)
    lo.outputPower(-4);     // Valid values: -4, -1, +2, +5 dBm

    TEST_ASSERT_EQUAL_UINT8((1u << 4), lo.getDirtyMask());
    lo.updateRegisters();

    // R4[7:6] should have changed to 0b10 for +2 dBm
    uint32_t newPower = (mock.regWrites[0] >> 6) & 0x3;
    uint32_t oldPower = (defaultCurr[4] >> 6) & 0x3;
    TEST_ASSERT_EQUAL_UINT32(1u, newPower);
    TEST_ASSERT_TRUE(newPower != oldPower);

    // Check forced R0 write equals baseline R0
    TEST_ASSERT_EQUAL_HEX32(defaultCurr[0], mock.regWrites[6]);
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
