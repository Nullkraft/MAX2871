/* bitbang_hal.h
   (MAX2871 DIY Evaluation board)

   Communicates from Arduino to max2871 using IO pins
 */

#ifndef BITBANG_HAL_H
#define BITBANG_HAL_H

#include "hal.h"
#include <Arduino.h>

class BitBangHAL : public HAL {
private:
    uint8_t pinCLK, pinDATA, pinLE;

public:
    BitBangHAL(uint8_t clkPin, uint8_t dataPin, uint8_t lePin)
        : pinCLK(clkPin), pinDATA(dataPin), pinLE(lePin) {}

    void begin() {
        pinMode(pinCLK, PINMODE_OUTPUT);
        pinMode(pinDATA, PINMODE_OUTPUT);
        pinMode(pinLE, PINMODE_OUTPUT);
        digitalWrite(pinCLK, PINLEVEL_LOW);
        digitalWrite(pinLE, PINLEVEL_LOW);
    }

    void pinMode(uint8_t pin, PinMode mode) override {
        ::pinMode(pin, mode == PINMODE_OUTPUT ? OUTPUT : INPUT);
    }

    void digitalWrite(uint8_t pin, PinLevel val) override {
        ::digitalWrite(pin, val == PINLEVEL_HIGH ? HIGH : LOW);
    }

    void spiWriteRegister(uint32_t regVal) override {
        digitalWrite(pinLE, PINLEVEL_LOW);
        for (int i = 31; i >= 0; --i) {
            digitalWrite(pinDATA, static_cast<PinLevel>((regVal >> i) & 1));
            digitalWrite(pinCLK, PINLEVEL_HIGH);
            delayMicroseconds(1);
            digitalWrite(pinCLK, PINLEVEL_LOW);
        }
        digitalWrite(pinLE, PINLEVEL_HIGH);
        delayMicroseconds(1);
        digitalWrite(pinLE, PINLEVEL_LOW);
    }

    // Unused virtual functions
    void spiBegin() {}
    void spiTransfer(uint8_t data) {}
    void setCEPin(bool enable) {}
    bool readMuxout() { return true; }      // TODO: Implement testing muxOut
};

#endif
