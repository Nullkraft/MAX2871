#ifndef _MAX2871_HAL_ARDUINO_
#define _MAX2871_HAL_ARDUINO_

#ifdef ARDUINO   // Only compile this if building for Arduino

#include <Arduino.h>
#include <SPI.h>
#include "max2871_hal.h"

class Max2871ArduinoHal : public Max2871Hal {
private:
    uint8_t lePin;
    uint8_t cePin;
    uint8_t muxPin;

public:
    Max2871ArduinoHal(uint8_t le, uint8_t ce, uint8_t mux)
        : lePin(le), cePin(ce), muxPin(mux) {}

    void begin() {
        pinMode(lePin, OUTPUT);
        pinMode(cePin, OUTPUT);
        pinMode(muxPin, INPUT);
        SPI.begin();
    }

    void spiWriteRegister(uint32_t value) override {
        digitalWrite(lePin, LOW);
        SPI.transfer((value >> 24) & 0xFF);
        SPI.transfer((value >> 16) & 0xFF);
        SPI.transfer((value >> 8) & 0xFF);
        SPI.transfer(value & 0xFF);
        digitalWrite(lePin, HIGH);  // latch enable pulse
    }

    void toggleLEPin() override {
        digitalWrite(lePin, LOW);
        delayMicroseconds(1);
        digitalWrite(lePin, HIGH);
    }

    void setCEPin(bool enabled) override {
        digitalWrite(cePin, enabled ? HIGH : LOW);
    }

    bool readMuxout() override {
        return digitalRead(muxPin);
    }
};

#endif // ARDUINO
#endif // _MAX2871_HAL_ARDUINO_
