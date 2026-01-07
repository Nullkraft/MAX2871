// smoke_hal.h  (no hardware, no Arduino core)
#ifndef SMOKE_HAL_H
#define SMOKE_HAL_H

#include "hal.h"

class SmokeHAL : public HAL {
public:
    explicit SmokeHAL(uint8_t /*pinLE*/) {}

    void doNothing() {}
    // --- SPI (no-op) ---
    void spiBegin() override {}
    // void spiTransfer(uint8_t /*data*/) override {}

    // --- GPIO (no-op) ---
    void pinMode(uint8_t /*pin*/, pin_mode /*mode*/) override {}
    void digitalWrite(uint8_t /*pin*/, pin_level /*val*/) override {}

    // Timing
    virtual void delayMs(uint32_t ms) override {}

    // MAX2871-specific
    virtual void spiWriteRegister(uint32_t value) override {}
    virtual void setCEPin(bool enable) override {}
    virtual bool readMuxout() override {}

    // ADC - ADS7826 (10-bit, returned left-justified in 12-bit field)
    virtual uint16_t readADC(ADCChannel channel = ADC_COARSE) override {}
};

#endif // SMOKE_HAL_H
