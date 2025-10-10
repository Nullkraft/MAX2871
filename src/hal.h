// hal.h
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

enum PinMode {
    PINMODE_INPUT,
    PINMODE_OUTPUT
};

enum PinLevel {
    PINLEVEL_LOW = 0,
    PINLEVEL_HIGH = 1
};

class HAL {
public:
    virtual ~HAL() {}

    // SPI
    virtual void spiBegin() = 0;
    virtual void spiTransfer(uint8_t data) = 0;

    // GPIO
    virtual void pinMode(uint8_t pin, PinMode mode) = 0;
    virtual void digitalWrite(uint8_t pin, PinLevel val) = 0;

    // MAX2871-specific extras
    virtual void spiWriteRegister(uint32_t value) = 0;  // convenience
    virtual void setCEPin(bool enable) = 0;
    virtual bool readMuxout() = 0;
};

#endif // HAL_H
