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

/* feather_hal.h
   Hardware Abstraction Layer for Adafruit Feather RP2040
   - Talks to MAX2871 over hardware SPI
   - Mirrors the ArduinoHAL interface used in your MAX2871 driver

   Notes:
   * RP2040 Arduino-Pico default SPI pins:
       SCK=18, MOSI=19, MISO=16 (SS is user-defined; we use LE as the latch)
   * MAX2871 allows high SPI clocks; start ~8–12 MHz and raise once stable.
*/

#ifndef FEATHER_HAL_H
#define FEATHER_HAL_H

#include <Arduino.h>
#include <SPI.h>
#include "hal.h"   // defines pin_mode, pin_level, HAL base class

class FeatherHAL : public HAL {
public:
    // Construct with required control pins. You can pass 0xFF for any unused pin.
    // le  = MAX2871 LE (latch enable)
    // ce  = MAX2871 CE (chip enable)
    // mux = MAX2871 MUXOUT pin for lock detect
    explicit FeatherHAL(uint8_t lePin, uint8_t cePin = 0xFF, uint8_t muxPin = 0xFF)
    : _le(lePin), _ce(cePin), _mux(muxPin) {}

    // Call once from setup(): config pins and SPI
    void begin() {
        // Basic pin config
        if (_le  != 0xFF) ::pinMode(_le, OUTPUT), ::digitalWrite(_le, LOW);
        if (_ce  != 0xFF) ::pinMode(_ce, OUTPUT), ::digitalWrite(_ce, LOW);   // keep LO off initially
        if (_mux != 0xFF) ::pinMode(_mux, INPUT);                             // MUXOUT (lock detect)

        // Start SPI on default bus (SCK18/MOSI19/MISO16)
        SPI.begin();
    }

    void spiBegin() override {
        // We'll begin a transaction around every 32-bit write.
        // (If you want to optimize, you can lift this out.)
    }

    // Optional: pick a faster/slower SPI clock (Hz). Call before beginTransaction.
    void setSpiClockHz(uint32_t hz) {
        _spiHz = hz;
    }

    // ---- HAL virtuals ----

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

    void digitalWrite(uint8_t pin, pin_level val) override {
        ::digitalWrite(pin, (val == PINLEVEL_HIGH) ? HIGH : LOW);
    }

    // Timing
    void delayMs(uint32_t ms) override { ::delay(ms); }

    // MAX2871 helpers
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
        return (::digitalRead(_mux) == HIGH);
    }

private:
    uint8_t _le;
    uint8_t _ce;
    uint8_t _mux;
    uint32_t _spiHz = 8000000UL;  // start conservatively; raise once validated
};

#endif // FEATHER_HAL_H
