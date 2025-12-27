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
    explicit ArduinoHAL(uint8_t lePin, uint8_t cePin = 0xFF, uint8_t muxPin = 0xFF)
        : _le(lePin), _ce(cePin), _mux(muxPin) {}

void begin() {
    // Basic pin config
    if (_le  != 0xFF) ::pinMode(_le, OUTPUT), ::digitalWrite(_le, LOW);
    if (_ce  != 0xFF) ::pinMode(_ce, OUTPUT), ::digitalWrite(_ce, LOW);
    if (_mux != 0xFF) ::pinMode(_mux, INPUT);

    SPI.begin();
}

    void spiBegin() override {
        // We’ll begin a transaction around every 32-bit write.
        // (If you want to optimize, you can lift this out.)
    }

    // Optional: pick a faster/slower SPI clock (Hz). Call before beginTransaction.
    void setSpiClockHz(uint32_t hz) { _spiHz = hz; }

    // GPIO
    void pinMode(uint8_t pin, pin_mode mode) override {
        switch (mode) {
            case pin_mode::PINMODE_INPUT:         ::pinMode(pin, INPUT); break;
            case pin_mode::PINMODE_OUTPUT:        ::pinMode(pin, OUTPUT); break;
            case pin_mode::PINMODE_INPUT_PULLUP:  ::pinMode(pin, INPUT_PULLUP); break;
            case pin_mode::PINMODE_INPUT_PULLDOWN:
#if defined(INPUT_PULLDOWN)
                ::pinMode(pin, INPUT_PULLDOWN);
#else
                ::pinMode(pin, INPUT);    // fallback
#endif
                break;
            default: ::pinMode(pin, INPUT); break;
        }
    }


    void delayMs(uint32_t ms) override {
        delay(ms);
    }

    void digitalWrite(uint8_t pin, pin_level val) override {
        ::digitalWrite(pin, (val == PINLEVEL_HIGH ? HIGH : LOW));
    }

    void spiWriteRegister(uint32_t value) override {
        // MAX2871 write (MSB first, 32 bits)
        SPISettings settings(_spiHz, MSBFIRST, SPI_MODE0);
        SPI.beginTransaction(settings);

        // Drive LE low for shift phase
        if (_le != 0xFF) ::digitalWrite(_le, LOW);

        // Transfer 32 bits MSB-first
        SPI.transfer16((value >> 16) & 0xFFFF);
        SPI.transfer16(value & 0xFFFF);

        // Latch on LE rising edge
        if (_le != 0xFF) {
            ::digitalWrite(_le, HIGH);
            ::digitalWrite(_le, LOW);
        }

        SPI.endTransaction();
    }

    void setCEPin(bool enable) override {
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
    uint32_t _spiHz = 8000000UL;    // Arduino Uno Max SPI speed = 8 Mbs
};

#endif // ARDUINO_HAL_H
