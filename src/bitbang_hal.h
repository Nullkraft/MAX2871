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
    uint8_t pinCLK, pinDATA, pinLE, pinCE;

public:
    BitBangHAL(uint8_t clkPin, uint8_t dataPin, uint8_t lePin, uint8_t cePin = 0xFF)
        : pinCLK(clkPin), pinDATA(dataPin), pinLE(lePin), pinCE(cePin) {}

    void begin() {
        pinMode(pinCLK, PINMODE_OUTPUT);
        pinMode(pinDATA, PINMODE_OUTPUT);
        pinMode(pinLE, PINMODE_OUTPUT);
        digitalWrite(pinCLK, PINLEVEL_LOW);
        digitalWrite(pinLE, PINLEVEL_LOW);
        if (pinCE != 0xFF) {
            ::pinMode(pinCE, OUTPUT);
            ::digitalWrite(pinCE, LOW);
        }
    }

    void pinMode(uint8_t pin, pin_mode mode) override {
        ::pinMode(pin, mode == PINMODE_OUTPUT ? OUTPUT : INPUT);
    }

    void digitalWrite(uint8_t pin, pin_level val) override {
        ::digitalWrite(pin, val == PINLEVEL_HIGH ? HIGH : LOW);
    }

    void delayMs(uint32_t ms) override {
        delay(ms);
    }

    void setCEPin(bool enable) override {
        if (pinCE == 0xFF) {
            return;
        }
        ::digitalWrite(pinCE, enable ? HIGH : LOW);
    }

    void spiWriteRegister(uint32_t regVal) override {
        shiftOut(pinDATA, pinCLK, MSBFIRST, regVal >> 24);
        shiftOut(pinDATA, pinCLK, MSBFIRST, regVal >> 16);
        shiftOut(pinDATA, pinCLK, MSBFIRST, regVal >> 8);
        shiftOut(pinDATA, pinCLK, MSBFIRST, regVal);
        digitalWrite(pinLE, PINLEVEL_HIGH);
        digitalWrite(pinLE, PINLEVEL_LOW);
    }

    // Unused virtual functions
    void spiBegin() {}
    bool readMuxout() { return true; }      // TODO: Implement testing muxOut
};

#endif
