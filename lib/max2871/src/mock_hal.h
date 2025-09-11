// mock_hal.h (Arduino-safe)
#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "hal.h"

class MockHAL : public HAL {
public:
    static constexpr uint8_t MAX_WRITES = 32;
    uint32_t regWrites[MAX_WRITES];
    uint8_t writeCount = 0; 

    void spiBegin() override {}
    void spiTransfer(uint8_t) override {}
    void pinMode(uint8_t, PinMode) override {}
    void digitalWrite(uint8_t, PinLevel) override {}

    void spiWriteRegister(uint32_t value) override {
        if (writeCount < MAX_WRITES) {
            regWrites[writeCount++] = value;
        }
    }
    void setCEPin(bool) override {}
    bool readMuxout() override { return false; }
};

#endif // MOCK_HAL_H
