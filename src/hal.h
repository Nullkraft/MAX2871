/* hal.h
    (Hardware Abstraction Layer)
    Purpose: Allows the use of different hardware platforms/communication methods
    without changing your code. It abstracts HOW you communicate with the chip:

    ArduinoHAL - Uses Arduino's hardware SPI peripheral
    FeatherRP2040 - Uses Raspberry PI's RP2040 hardware SPI peripheral
    BitBangHAL - Uses bit-banging on GPIO pins (software SPI)
    MockHAL - Fake hardware for testing on PC

    All talking to the SAME hardware, just different ways to send bits!

    (c) 2025 Mark Stanley, GPL-3.0-or-later
 */
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

enum pin_mode { PINMODE_INPUT, PINMODE_INPUT_PULLUP, PINMODE_INPUT_PULLDOWN, PINMODE_OUTPUT };
enum pin_level { PINLEVEL_LOW = 0, PINLEVEL_HIGH = 1 };
enum ADCChannel { ADC_COARSE = 1, ADC_FINE = 2 };

// --- Bitfield Utilities ---
// Helper functions for register bit manipulation

// Return a mask covering bits [bit_lo : bit_hi], inclusive.
// Example: bitMask(12, 5) -> 0b00011111100000
inline uint32_t bitMask(uint8_t bit_hi, uint8_t bit_lo) {
    if (bit_hi > 31 || bit_lo > bit_hi) return 0;
    return ((0xFFFFFFFFu >> (31 - (bit_hi - bit_lo))) << bit_lo);
}

// Return 'value' aligned into field [bit_lo : bit_hi], with other bits zero.
// Example: fieldValue(0b101, 7, 5) -> 0b00000000000000000000001010000000
inline uint32_t fieldValue(uint32_t value, uint8_t bit_hi, uint8_t bit_lo) {
    if (bit_hi > 31 || bit_lo > bit_hi) return 0;
    uint32_t mask = bitMask(bit_hi, bit_lo);
    return (value << bit_lo) & mask;
}


class HAL {
public:
    virtual ~HAL() {}

    // SPI
    virtual void spiBegin() = 0;

    // GPIO
    virtual void pinMode(uint8_t pin, pin_mode mode) = 0;
    virtual void digitalWrite(uint8_t pin, pin_level val) = 0;

    // Timing
    virtual void delayMs(uint32_t ms) = 0;

    // MAX2871-specific
    virtual void spiWriteRegister(uint32_t value) = 0;
    virtual void setCEPin(bool enable) = 0;
    virtual bool readMuxout() = 0;

    // ADC - ADS7826 (10-bit, returned left-justified in 12-bit field)
    virtual uint16_t readADC(ADCChannel channel) = 0;
};

#endif // HAL_H
