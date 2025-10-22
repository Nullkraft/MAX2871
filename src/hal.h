/* hal.h
    (Hardware Abstraction Layer)
    Purpose: Allows the use of different hardware platforms/communication methods
    without changing your code. It abstracts HOW you communicate with the chip:

    ArduinoHAL - Uses Arduino's hardware SPI peripheral
    BitBangHAL - Uses bit-banging on GPIO pins (software SPI)
    MockHAL - Fake hardware for testing on PC

    All talking to the SAME chip (MAX2871), just different ways to send bits!
 */
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

enum PinMode { PINMODE_INPUT, PINMODE_OUTPUT };
enum PinLevel { PINLEVEL_LOW = 0, PINLEVEL_HIGH = 1 };

class HAL {
public:
    virtual ~HAL() {}

    // SPI
    virtual void spiBegin() = 0;

    // GPIO
    virtual void pinMode(uint8_t pin, PinMode mode) = 0;
    virtual void digitalWrite(uint8_t pin, PinLevel val) = 0;

    // Timing
    virtual void delayMs(uint32_t ms) = 0;

    // MAX2871-specific extras
    virtual void spiWriteRegister(uint32_t value) = 0;  // convenience
    virtual void setCEPin(bool enable) = 0;

    // Method that MAX2871::isLocked() uses internally
    virtual bool readMuxout() = 0;
};

#endif // HAL_H
