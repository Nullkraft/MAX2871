/* mcu_hal.h
   Pure MCU abstraction for generic board services.

   This interface intentionally stays below any MAX2871-specific semantics.
   It covers GPIO, delay, and SPI primitives that belong to the controller
   side of the design rather than to the synthesizer chip.
 */

#ifndef MCU_HAL_H
#define MCU_HAL_H

#include <stdint.h>
#include "hal.h"

class IDelayProvider {
public:
    virtual ~IDelayProvider() {}
    virtual void delayMs(uint32_t ms) = 0;
};

class IMCUHAL : public IDelayProvider {
public:
    virtual ~IMCUHAL() {}

    // GPIO
    virtual void pinMode(uint8_t pin, pin_mode mode) = 0;
    virtual void digitalWrite(uint8_t pin, pin_level val) = 0;
    virtual int digitalRead(uint8_t pin) = 0;

    // SPI primitives
    virtual void spiBegin() = 0;
    virtual void spiEnd() = 0;
    virtual void spiTransfer16(uint16_t value) = 0;
};

#endif // MCU_HAL_H
