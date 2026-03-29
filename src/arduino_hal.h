/* arduino_hal.h
   (Spectrum Analyzer Production board)

   Communicates from Arduino to the MAX2871 over the SPI bus (Mode 0).

   Caller is responsible for setting appropriate SPI clock rate
   before each MAX2871 operation via setSpiClockHz().

   (c) 2025 Mark Stanley, GPL-3.0-or-later
 */

#ifndef ARDUINO_HAL_H
#define ARDUINO_HAL_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"
#include "mcu_hal.h"
#include "max2871_transport.h"

class ArduinoHAL : public IMCUHAL, public I_MAX2871Transport {
public:
    // Construct with required control pins. You can pass 0xFF for any unused pin.
    // le  = MAX2871 LE (latch enable)
    // ce  = MAX2871 CE (chip enable)
    // mux = MAX2871 MUXOUT pin for lock detect
    explicit ArduinoHAL(uint8_t lePin, uint8_t cePin = 0xFF, uint8_t muxPin = 0xFF)
        : _le(lePin), _ce(cePin), _mux(muxPin) {}

    void begin() {
        // Basic pin config
        if (_le  != 0xFF) ::pinMode(_le, OUTPUT), ::digitalWrite(_le, LOW);
        if (_ce  != 0xFF) ::pinMode(_ce, OUTPUT), ::digitalWrite(_ce, LOW);   // keep LO off initially
        if (_mux != 0xFF) ::pinMode(_mux, INPUT);                             // MUXOUT (lock detect)

        // Start SPI on the default bus
        SPI.begin();
    }

    // Optional: pick a faster/slower SPI clock (Hz). Call before beginTransaction.
    void setSpiClockHz(uint32_t hz) { _spiHz = hz; }

    // GPIO
    void pinMode(uint8_t pin, pin_mode mode) override {
        switch (mode) {
            case pin_mode::PINMODE_INPUT:           ::pinMode(pin, INPUT); break;
            case pin_mode::PINMODE_OUTPUT:          ::pinMode(pin, OUTPUT); break;
            case pin_mode::PINMODE_INPUT_PULLUP:    ::pinMode(pin, INPUT_PULLUP); break;
            case pin_mode::PINMODE_INPUT_PULLDOWN:  ::pinMode(pin, HAL_INPUT_PULLDOWN); break;
            default: ::pinMode(pin, INPUT); break;
        }
    }

    void digitalWrite(uint8_t pin, pin_level val) override {
        ::digitalWrite(pin, (val == PINLEVEL_HIGH ? HIGH : LOW));
    }

    int digitalRead(uint8_t pin) override {
        return ::digitalRead(pin);
    }

    // Timing
    void delayMs(uint32_t ms) override { ::delay(ms); }

    void spiBegin() override {
        SPI.beginTransaction(SPISettings(_spiHz, MSBFIRST, SPI_MODE0));
    }

    void spiEnd() override {
        SPI.endTransaction();
    }

    void spiTransfer16(uint16_t value) override {
        SPI.transfer16(value);
    }

    // MAX2871 helpers
    void spiWriteRegister(uint32_t value) override {
        // MAX2871 write (MSB first, 32 bits, Mode 0)
        spiBegin();
        ::digitalWrite(_le, LOW);
        spiTransfer16((value >> 16) & 0xFFFF);
        spiTransfer16(value & 0xFFFF);
        ::digitalWrite(_le, HIGH);
        ::digitalWrite(_le, LOW);
        spiEnd();
    }

    void setCEPin(bool enable) {
        if (_ce != 0xFF) {
            ::digitalWrite(_ce, enable ? HIGH : LOW);
        }
    }

    bool readMuxout() override {
        if (_mux == 0xFF) return false;
        return ::digitalRead(_mux) == HIGH;
    }

private:
    uint8_t _le;
    uint8_t _ce;
    uint8_t _mux;
    uint32_t _spiHz = 8000000UL;    // Default: Arduino Uno max = 8 MHz
};

#endif // ARDUINO_HAL_H
