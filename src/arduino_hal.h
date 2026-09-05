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

class ArduinoHAL : public IDelayProvider, public I_MAX2871Transport {
public:
    // Construct with optional MUXOUT pin for lock detect.
    explicit ArduinoHAL(uint8_t, uint8_t = 0xFF, uint8_t muxPin = 0xFF)
        : _mux(muxPin) {}

    void begin() {
    }

    // Optional: pick a faster/slower SPI clock (Hz). Call before beginTransaction.
    void setSpiClockHz(uint32_t hz) { _spiHz = hz; }

    // Timing
    void delayMs(uint32_t ms) override { ::delay(ms); }

    // MAX2871 helpers
    void spiWriteRegister(uint32_t value) override {
        // MAX2871 register payload transfer (MSB first, 32 bits).
        SPI.transfer16((value >> 16) & 0xFFFF);
        SPI.transfer16(value & 0xFFFF);
    }

    bool readMuxout() override {
        if (_mux == 0xFF) return false;
        return ::digitalRead(_mux) == HIGH;
    }

private:
    uint8_t _mux;
    uint32_t _spiHz = 8000000UL;    // Default: Arduino Uno max = 8 MHz
};

#endif // ARDUINO_HAL_H
