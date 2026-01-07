/* arduino_hal.h
   (Spectrum Analyzer Production board)

   Communicates from Arduino to peripherals over the SPI bus:
   - MAX2871 PLL synthesizers (Mode 0)
   - ADS7826 ADCs (Mode 1)

   Caller is responsible for setting appropriate SPI clock rate
   before each device operation via setSpiClockHz().

   (c) 2025 Mark Stanley, GPL-3.0-or-later
 */

#ifndef ARDUINO_HAL_H
#define ARDUINO_HAL_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"

class ArduinoHAL : public HAL {
public:
    explicit ArduinoHAL(uint8_t lePin, uint8_t cePin = 0xFF, uint8_t muxPin = 0xFF,
                        uint8_t selAdc1 = 0xFF, uint8_t selAdc2 = 0xFF)
        : _le(lePin), _ce(cePin), _mux(muxPin),
          _selAdc1(selAdc1), _selAdc2(selAdc2) {}

    void begin() {
        // MAX2871 pins
        if (_le  != 0xFF) ::pinMode(_le, OUTPUT), ::digitalWrite(_le, LOW);
        if (_ce  != 0xFF) ::pinMode(_ce, OUTPUT), ::digitalWrite(_ce, LOW);
        if (_mux != 0xFF) ::pinMode(_mux, INPUT);

        // ADC chip selects (active-low, start deselected)
        if (_selAdc1 != 0xFF) ::pinMode(_selAdc1, OUTPUT), ::digitalWrite(_selAdc1, HIGH);
        if (_selAdc2 != 0xFF) ::pinMode(_selAdc2, OUTPUT), ::digitalWrite(_selAdc2, HIGH);

        SPI.begin();
    }

    void spiBegin() override {
        // Transaction started around each device access
    }

    // Set SPI clock rate before device operations
    // e.g. setSpiClockHz(lo1.spiHz) or setSpiClockHz(adc.spiHz)
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
        // MAX2871 write (MSB first, 32 bits, Mode 0)
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

    uint16_t readADC(ADCChannel channel = ADC_COARSE) override {
        // ADS7826: 10-bit ADC, SPI Mode 1
        // Caller must set appropriate SPI clock rate before calling (max 2.8 MHz)
        // Returns 10-bit data left-justified in 12-bit field
        uint8_t csPin = (channel == ADC_COARSE) ? _selAdc1 : _selAdc2;
        if (csPin == 0xFF) return 0;

        SPISettings settings(_spiHz, MSBFIRST, SPI_MODE1);
        SPI.beginTransaction(settings);

        ::digitalWrite(csPin, LOW);
        uint16_t raw = SPI.transfer16(0);
        ::digitalWrite(csPin, HIGH);

        SPI.endTransaction();

        // Bit alignment: 2 sample clocks, 1 null, 10 data bits, 3 trailing
        // Data sits in bits [12:3], shift right 1 and mask to left-justify in 12-bit field
        return (raw >> 1) & 0x0FFC;
    }

private:
    uint8_t _le;
    uint8_t _ce;
    uint8_t _mux;
    uint8_t _selAdc1;
    uint8_t _selAdc2;
    uint32_t _spiHz = 8000000UL;    // Default: Arduino Uno max = 8 MHz
};

#endif // ARDUINO_HAL_H
