/* arduino_hal.h
   (Spectrum Analyzer Production board)

   Communicates from Arduino to max2871 over the SPI bus
 */

#ifndef ARDUINO_HAL_H
#define ARDUINO_HAL_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"

class ArduinoHAL : public HAL {
public:
    ArduinoHAL(uint8_t lePin, uint8_t cdPin = 0xFF, uint8_t muxPin = 0xFF)
    : _le(lePin), _ce(cdPin), _mux(muxPin) {}

    void begin(uint32_t spiHz = 20000000UL) {
        _spiHz = spiHz;
        _cp = A1;
        _dp = A2;
        ::pinMode(_le, OUTPUT);
        ::digitalWrite(_le, HIGH);    // LE Pin active-low
        if (_ce != 0xFF) {
            ::pinMode(_ce, OUTPUT);
            ::digitalWrite(_ce, LOW);
        }
        if (_mux != 0xFF) {
            ::pinMode(_mux, INPUT);
        }
        SPI.begin();
    }

    void spiBegin() override {
        // We’ll begin a transaction around every 32-bit write.
        // (If you want to optimize, you can lift this out.)
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
        ::digitalWrite(_le, LOW);
        for (int i = 3; i >= 0; --i) {
            SPI.transfer((value >> (i * 8)) & 0xFF);
        }
        ::digitalWrite(_le, HIGH);
        SPI.endTransaction();
    }

    void setCEPin(bool enable) override {
        if (_ce != 0xFF) {
            ::digitalWrite(_ce, enable ? HIGH : LOW);
        }
    }

    bool readMuxout() override {
        if (_mux != 0xFF) return false;
        return ::digitalRead(_mux) == HIGH;
    }

private:
    uint8_t _le;
    uint8_t _cp;
    uint8_t _dp;
    uint8_t _ce;
    uint8_t _mux;
    uint32_t _spiHz = 20000000UL;
};

#endif // ARDUINO_HAL_H
