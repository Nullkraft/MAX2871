/* I_PLLSynthesizer.h
    (Common PLL Synthesizer Interface)
    Abstract Interface for controlling different manufacturers PLL chips:

    MAX2871 - Maxim's PLL chip (23.5-6000 MHz)
    ADF4356 - Analog Devices PLL chip
    LMX2594 - Texas Instruments PLL chip

    All doing the same job (frequency synthesis), just different chips!

    (c) 2025 Mark Stanley, GPL-3.0-or-later
 */
#ifndef I_PLLSYNTHESIZER_H
#define I_PLLSYNTHESIZER_H

#include <stdint.h>
#include "hal.h"

class I_PLLSynthesizer {
public:
    virtual ~I_PLLSynthesizer() {}

    //  Initialization 
    virtual void attachHal(HAL* halptr) = 0;        // inject HAL
    virtual void begin(uint8_t lePin) = 0;

    //  Frequency Control 
    virtual void setFrequency(double freqMHz) = 0;              // calculates FMN+DIVA
    virtual void setFrequency(uint32_t fmn, uint8_t diva) = 0;  // bypass math

    //  Output Control 
    virtual void outputSelect(uint8_t sel) = 0;     // A, B, both, or off
    virtual void outputPower(int dBm) = 0;          // -4, -1, +2, +5 dBm
    virtual void outputEnable(uint8_t rfEn) = 0;    // Enable RF output

    //  Mode Control 
    virtual void mode(uint8_t type) = 0;            // 0=int-N, 1=frac-N

    //  Status 
    virtual bool isLocked() = 0;
};

#endif // I_PLLSYNTHESIZER_H
