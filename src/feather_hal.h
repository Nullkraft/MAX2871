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
    // cp  = Optional RF power enable for board (active-high), else 0xFF
    // dp  = Optional on-board power enable (active-high), else 0xFF
    // mux = MAX2871 MUXOUT pin for lock detect
    explicit FeatherHAL(uint8_t le, uint8_t ce, uint8_t cp, uint8_t dp, uint8_t mux,
                        SPIClass &spi = SPI)
        : _le(le), _ce(ce), _cp(cp), _dp(dp), _mux(mux), _spi(spi) {}

    // Call once from setup(): config pins and SPI
    void begin() {
        // Basic pin config
        if (_le  != 0xFF) ::pinMode(_le, OUTPUT), ::digitalWrite(_le, LOW);
        if (_ce  != 0xFF) ::pinMode(_ce, OUTPUT), ::digitalWrite(_ce, LOW);   // keep LO off initially
        if (_cp  != 0xFF) ::pinMode(_cp, OUTPUT), ::digitalWrite(_cp, LOW);   // board RF power off
        if (_dp  != 0xFF) ::pinMode(_dp, OUTPUT), ::digitalWrite(_dp, LOW);   // board power off
        if (_mux != 0xFF) ::pinMode(_mux, INPUT);                             // MUXOUT (lock detect)

        // Start SPI on default bus (SCK18/MOSI19/MISO16)
        _spi.begin();
    }

    // Optional: pick a faster/slower SPI clock (Hz). Call before beginTransaction.
    void setSpiClockHz(uint32_t hz) { _spiHz = hz; }

    // ---- HAL virtuals ----

    // GPIO
    void pinMode(uint8_t pin, pin_mode mode) override {
        switch (mode) {
            case pin_mode::PINMODE_INPUT:         ::pinMode(pin, INPUT); break;
            case pin_mode::PINMODE_OUTPUT:        ::pinMode(pin, OUTPUT); break;
            case pin_mode::PINMODE_INPUT_PULLUP:  ::pinMode(pin, INPUT_PULLUP); break;
            case pin_mode::input_pulldown:
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
        ::digitalWrite(pin, (val == pin_level::high) ? HIGH : LOW);
    }

    // Timing
    void delayMs(uint32_t ms) override { ::delay(ms); }

    // MAX2871 helpers
    void spiWriteRegister(uint32_t value) override {
        // MAX2871 latches on LE rising edge after shifting 32 bits MSB-first
        SPISettings settings(_spiHz, MSBFIRST, SPI_MODE0);
        _spi.beginTransaction(settings);

        // Drive LE low for shift phase
        if (_le != 0xFF) ::digitalWrite(_le, LOW);

        // Transfer 32 bits MSB-first
        // Split into 4 bytes [31:24], [23:16], [15:8], [7:0]
        uint8_t b3 = (value >> 24) & 0xFF;
        uint8_t b2 = (value >> 16) & 0xFF;
        uint8_t b1 = (value >> 8)  & 0xFF;
        uint8_t b0 = (value >> 0)  & 0xFF;
        _spi.transfer(b3);
        _spi.transfer(b2);
        _spi.transfer(b1);
        _spi.transfer(b0);

        // Latch on LE rising edge
        if (_le != 0xFF) {
            ::digitalWrite(_le, HIGH);
            // short hold; RP2040 is fast—ensure LE high > tLEH; a few hundred ns is fine
            // delayMicroseconds(1);  // uncomment if needed
            ::digitalWrite(_le, LOW);
        }

        _spi.endTransaction();
    }

    void setCEPin(bool enable) override {
        if (_ce != 0xFF) ::digitalWrite(_ce, enable ? HIGH : LOW);
    }

    bool readMuxout() override {
        if (_mux == 0xFF) return false;
        return (::digitalRead(_mux) == HIGH);
    }

    // Convenience board-power toggles if you wired them
    void setBoardPower(bool on) {
        if (_dp != 0xFF) ::digitalWrite(_dp, on ? HIGH : LOW);
    }
    void setRfPower(bool on) {
        if (_cp != 0xFF) ::digitalWrite(_cp, on ? HIGH : LOW);
    }

private:
    uint8_t _le;
    uint8_t _ce;
    uint8_t _cp;   // optional board RF power enable
    uint8_t _dp;   // optional general board power enable
    uint8_t _mux;

    SPIClass &_spi;
    uint32_t _spiHz = 8000000UL;  // start conservatively; raise once validated
};

#endif // FEATHER_HAL_H
