// smoke_hal.h  (no hardware, no Arduino core)
#ifndef SMOKE_HAL_H
#define SMOKE_HAL_H

#include "hal.h"
#include "mcu_hal.h"
#include "max2871_transport.h"

class SmokeHAL : public IMCUHAL, public I_MAX2871Transport {
public:
    explicit SmokeHAL(uint8_t /*pinLE*/) {}

    // --- GPIO (no-op) ---
    void pinMode(uint8_t /*pin*/, pin_mode /*mode*/) override {}
    void digitalWrite(uint8_t /*pin*/, pin_level /*val*/) override {}
    int digitalRead(uint8_t /*pin*/) override { return 0; }

    // Timing
    virtual void delayMs(uint32_t ms) override {}

    void spiBegin() override {}
    void spiEnd() override {}
    void spiTransfer16(uint16_t /*value*/) override {}

    // MAX2871-specific
    virtual void spiWriteRegister(uint32_t value) override {}
    void setCEPin(bool enable) {}
    virtual bool readMuxout() override { return false; }
};

#endif // SMOKE_HAL_H
