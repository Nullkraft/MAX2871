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
static BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE);
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

// Program a single register of the selected LO by sending and latching 4 bytes
void spiWriteLO(uint32_t reg, uint8_t selectPin) {
    digitalWrite(selectPin, LOW);
    shiftOut(PIN_DAT, PIN_CLK, MSBFIRST, reg >> 24);
    shiftOut(PIN_DAT, PIN_CLK, MSBFIRST, reg >> 16);
    shiftOut(PIN_DAT, PIN_CLK, MSBFIRST, reg >> 8);
    shiftOut(PIN_DAT, PIN_CLK, MSBFIRST, reg);
    delayMicroseconds(10);
    digitalWrite(selectPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(selectPin, LOW);
}

void writeToRegisters(uint8_t csPin) {
    hal.spiWriteRegister(lo.Curr.Reg[5]);    // First we program LO Register 5
    // delay(20);  // wait 20 mSec for MAX2871 internal capacitors to charge
    hal.spiWriteRegister(lo.Curr.Reg[4]); // Program remaining registers
    hal.spiWriteRegister(lo.Curr.Reg[3]); // Program remaining registers
    hal.spiWriteRegister(lo.Curr.Reg[2]); // Program remaining registers
    hal.spiWriteRegister(lo.Curr.Reg[1]); // Program remaining registers
    hal.spiWriteRegister(lo.Curr.Reg[0]); // Program remaining registers
    // delay(1);                               // Short delay before reading Register 6
    // hal.spiWriteRegister(lo.Curr.Reg[6], csPin);  // Tri-stating the mux output disables LO2 lock detect
}

void test_init_chip_50MHz_for_scope(void) {
    lo.attachHal(&hal);
    hal.begin();
    lo.begin(PIN_LE);
    // Program ~50.00 MHz
    // lo.setFrequency(50.0);
    writeToRegisters(PIN_LE);
    lo.outputEnable(5);

    // for (int i = 5; i >= 0; --i) {
    //     hal.ioWriteRegister(lo.Curr.Reg[i]);   // program the chip
    //     print_hex(lo.Curr.Reg[i]);
    // }
    print_registers(lo);
    TEST_MESSAGE("Set MAX2871 to ~66.000 MHz. Check RFOUTA/B on the scope.");
    // delay(5000);
    TEST_PASS();
}

int runUnityTests() {
    UNITY_BEGIN();
    RUN_TEST(test_init_chip_50MHz_for_scope);
    return UNITY_END();
}

void setup() {
    delay(2000);     // give serial monitor time to connect
    runUnityTests();
}

void loop() {}

#endif // ARDUINO
// #endif // if not ARDUINO
