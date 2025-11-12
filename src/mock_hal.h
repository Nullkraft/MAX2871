/* mock_hal.h (Arduino-safe)
   (Implements fake calls to hardware)

   Fakes communication
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "hal.h"

class MockHAL : public HAL {
public:
    static const int LOG_SIZE = 16;
    uint32_t writeLog[LOG_SIZE];
    static constexpr uint8_t MAX_WRITES = 7;
    uint32_t regWrites[MAX_WRITES];
    uint8_t writeCount = 0;

    void delayMs(uint32_t ms) override {
        // delay(ms);
    }

    void spiBegin() override {}
    void pinMode(uint8_t, pin_mode) override {}
    void digitalWrite(uint8_t, pin_level) override {}

    void spiWriteRegister(uint32_t value) override {
        if (writeCount < MAX_WRITES) {
            regWrites[writeCount++] = value;
        }
    }

    void logRegister(uint32_t value) {
        if (writeCount < LOG_SIZE) {
            writeLog[writeCount++] = value;
        }
    }
    
    void setCEPin(bool) override {}
    bool readMuxout() override { return false; }
};

#endif // MOCK_HAL_H
