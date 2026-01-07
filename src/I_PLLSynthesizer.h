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

// Support chips with up to 4 RF outputs
enum RFOutPort { RFNONE = 0, RF_A = 1, RF_B = 2, RF_C = 4, RF_D = 8, RF_ALL = 0xFF };

class I_PLLSynthesizer {
public:
    virtual ~I_PLLSynthesizer() {}

    //  Initialization
    virtual void begin() = 0;

    //  Frequency Control 
    virtual void setFrequency(double freqMHz) = 0;              // calculates FMN+DIVA
    virtual void setFrequency(uint32_t fmn, uint8_t diva) = 0;  // bypass math

    //  Output Control 
    virtual void outputSelect(RFOutPort port) = 0;     // A, B, both, or off
    virtual void outputPower(int dBm, RFOutPort port = RF_ALL) = 0;  // -4, -1, +2, +5 dBm

    //  Status 
    virtual bool isLocked() = 0;
};

#endif // I_PLLSYNTHESIZER_H
