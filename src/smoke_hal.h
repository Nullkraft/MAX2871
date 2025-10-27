// smoke_hal.h  (no hardware, no Arduino core)
#ifndef SMOKE_HAL_H
#define SMOKE_HAL_H

#include "hal.h"

class SmokeHAL : public HAL {
public:
    explicit SmokeHAL(uint8_t /*pinLE*/) {}

    // --- SPI (no-op) ---
    void spiBegin() override {}
    // void spiTransfer(uint8_t /*data*/) override {}

    // --- GPIO (no-op) ---
    void pinMode(uint8_t /*pin*/, PinMode /*mode*/) override {}
    void digitalWrite(uint8_t /*pin*/, PinLevel /*val*/) override {}
};

#endif // SMOKE_HAL_H
