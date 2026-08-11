/* mock_hal.h (Arduino-safe)
   (Implements fake calls to hardware)

   Fakes communication
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "hal.h"
#include "mcu_hal.h"
#include "max2871_transport.h"

class MockHAL : public IMCUHAL, public I_MAX2871Transport {
public:
    static constexpr uint8_t MAX_WRITES = 7;
    uint32_t regWrites[MAX_WRITES];
    uint8_t writeCount = 0;

    void delayMs(uint32_t ms) override {
        // delay(ms);
    }

    void pinMode(uint8_t, pin_mode) override {}
    void digitalWrite(uint8_t, pin_level) override {}
    int digitalRead(uint8_t) override { return 0; }

    void spiBegin() override {}
    void spiEnd() override {}
    void spiTransfer16(uint16_t) override {}

    void spiWriteRegister(uint32_t value) override {
        if (writeCount < MAX_WRITES) {
            regWrites[writeCount++] = value;
        }
    }

    void setCEPin(bool) {}
    bool readMuxout() override { return false; }
};

#endif // MOCK_HAL_H
