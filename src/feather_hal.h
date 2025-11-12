/* feather_hal.h
    (Spectrum Analyzer Production board)

    Communicates from Feather RP2040 to max2871 over the SPI bus

    Feather Pins:
    -----------------------------------
    SEL_RAM         A0  // Active Low
    SEL_FLASH       A1  // Active Low
    MUX             A2
    SEL_LO1         A3
    SEL_LO3         24  // D24
    SEL_ATTEN       25  // D25
    SEL_ADC_2        5  // D5 Active Low
    SEL_ADC_1        6  // D6 Active Low
    SEL_LO2          9  // D9
    REF_EN1         10  // D10
    REF_EN2         11  // D11
    POWER_DETECT    12  // D12 RF board power on/off detection (read-only)

    Initiallly derived from arduino_hal.h
 */

#ifndef FEATHER_HAL_H
#define FEATHER_HAL_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"

class FeatherHAL : public HAL {
public:
    FeatherHAL(uint8_t lePin, uint8_t cdPin = 0xFF, uint8_t muxPin = 0xFF)
    : _le(lePin), _ce(cdPin), _mux(muxPin) {}

    void begin(uint32_t spiHz = 8000000UL) {
        _spiHz = spiHz;
        _cp = A1;
        _dp = A2;
        ::pinMode(_le, OUTPUT);
        ::digitalWrite(_le, HIGH);
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

    void pinMode(uint8_t pin, pin_mode mode) override {
        ::pinMode(pin, (mode == PINMODE_OUTPUT ? OUTPUT : INPUT));
    }

    void delayMs(uint32_t ms) override {
        delay(ms);
    }

    void digitalWrite(uint8_t pin, PinLevel val) override {
        ::digitalWrite(pin, (val == PINLEVEL_HIGH ? HIGH : LOW));
    }

    void spiWriteRegister(uint32_t value) override {
        // MAX2871 write (MSB first, 32 bits)
        SPI.beginTransaction(SPISettings(_spiHz, MSBFIRST, SPI_MODE0));
        SPI.transfer16((value >> 16) & 0xFFFF);
        SPI.transfer16(value & 0xFFFF);
        ::digitalWrite(_le, HIGH);          // HIGH then ...
        ::digitalWrite(_le, LOW);           // LOW to program the chip
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
    uint32_t _spiHz = 8000000UL;    // Feather Uno Max SPI speed = ??? Mbs
};

#endif // FEATHER_HAL_H
