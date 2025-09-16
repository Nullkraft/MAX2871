#ifndef HAL_ARDUINO_H
#define HAL_ARDUINO_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"

class ArduinoHAL : public HAL {
public:
    void spiBegin() override {
        SPI.begin();
    }

    void spiTransfer(uint8_t data) override {
        SPI.transfer(data);
    }

    void pinMode(uint8_t pin, PinMode mode) override {
        ::pinMode(pin, (mode == PINMODE_OUTPUT ? OUTPUT : INPUT));
    }

    void digitalWrite(uint8_t pin, PinLevel val) override {
        ::digitalWrite(pin, (val == PINLEVEL_HIGH ? HIGH : LOW));
    }

    void spiWriteRegister(uint32_t value) override {
        // Typical MAX2871 write (MSB first, 32 bits)
        SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
        for (int i = 3; i >= 0; --i) {
            SPI.transfer((value >> (i * 8)) & 0xFF);
        }
        SPI.endTransaction();
    }

    void setCEPin(bool enable) override {
        // User decides which pin is CE; could be set via constructor
        // For now, left as TODO
    }

    bool readMuxout() override {
        // User must assign MUXOUT pin
        // For now, return false
        return false;
    }
};

#endif // HAL_ARDUINO_H
