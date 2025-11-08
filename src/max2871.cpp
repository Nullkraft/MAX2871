#include "max2871.h"

// ---- Static read-only defaults ----
const MAX2871::max2871Registers MAX2871::defaultRegisters = {{
    /* 50.0 MHz target with 60.0 MHz refClock
     * Use for max2871 evaluation board
     */
    // 0x001D1740, // R0
    // 0x4000FFE1, // R1
    // 0x80005F42, // R2
    // 0x00009F23, // R3
    // 0x63EE83FC, // R4    RF_A and RF_B on @ +5 dBm // DIVA = div-by-64
    // 0x00400005, // R5

    /* 60.0 MHz target with 66.0 Mhz refClock
     * User for Spectrum Analyzer RF board
     */
    0x001D1740,  // Register 0 - Contains F and parts of N
    0x40017FE1,  // Register 1 - Contains M
    0x80005F42,  // Register 2 - Digital Lock Detect (DLD) on
    0x00001F23,  // Register 3
    0x63EE81FC,  // Register 4  RF_A off // RF_B @ +5 dBm // DIVA = div-by-64
    0x00400005   // Register 5
}};

// ---- Construction ----

/* hal defaults to nullptr */
MAX2871::MAX2871(double refIn) 
    : refInHz(refIn), fpfdHz(0.0), _rfEnPin(0), first_init(true), _dirtyMask(0) {
}

void MAX2871::begin() {
    reset();  // Fill the working (shadow) registers
}

// ---- Frequency Control ----

void MAX2871::setFrequency(double freqMHz) {
    freq2FMN(freqMHz);
    setRegisterField(1, 14,  3, M);
    setRegisterField(0, 14,  3, Frac);
    setRegisterField(0, 30, 15, N);
    setRegisterField(4, 22, 20, DIVA);
    updateRegisters();
}

void MAX2871::setFrequency(uint32_t fmn, uint8_t diva) {
    Frac = (fmn >> 20) & 0xFFF;
    M = (fmn >> 8) & 0xFFF;
    N = fmn & 0xFF;
    DIVA = diva;
    float freq = fmn2freq();
    setFrequency(freq);
}

void MAX2871::freq2FMN(float target_freq_MHz) {
    float floatFrac;
    R = 1;
    Fpfd = refInHz / R;                // Phase Frequency Detector input frequency
    float max_error = pow(2, 32);      // Large initial error
    float Fvco = target_freq_MHz;

    // Adjust Fvco to be within 3000 to 6000 MHz range and calculate DIVA accordingly
    DIVA = 0;                           // Re-initialize DIVA (divide by 1)
    while (Fvco < 3000.0) {
        Fvco *= 2;                      // Double until VCO is in valid range
        DIVA += 1;                      // Track doublings to determine DIVA
    }

    float NdotF = Fvco / Fpfd;
    N = static_cast<uint8_t>(NdotF);    // Integer portion (N of NdotF)
    floatFrac = NdotF - N;              // Fractional portion
    uint16_t best_F = 0;
    uint16_t best_M = 0;

    // Loop through M from 4095 down to 2
    for (uint16_t M_candidate = 4095; M_candidate > 1; --M_candidate) {
        float F_candidate = static_cast<uint16_t>(floatFrac * float(M_candidate));
        float FvcoCalculated = Fpfd * (N + F_candidate / M_candidate);
        float err = fabs(Fvco - FvcoCalculated);
        if (err == 0) {
            best_F = F_candidate;       // Perfect match found
            best_M = M_candidate;
            break;
        }
        if (err <= max_error) {
            max_error = err;
            best_F = F_candidate;
            best_M = M_candidate;
        }
    }

    Frac = static_cast<uint16_t>(round(best_F));
    M = best_M;
}

double MAX2871::fmn2freq() {
    double fVCO = Fpfd * (N + (double)Frac / M);
    double fout = fVCO / (1 << DIVA);
    return fout;
}

// ---- Output Control ----

// RFOutPort = RFNONE, RF_A, RF_B or RF_ALL
void MAX2871::outputSelect(RFOutPort port) {
    /* R4[8] disables RFoutB when 0, R4[5] disables RFoutA when 0.
     * R4[8] == 1 when B enabled; R4[8] == 0 when B disabled
     *
     *  sel |  A  |  B  |
     * -----+-----+-----+
     *   0  | off | off |
     * -----+-----+-----+
     *   1  | off | on  |
     * -----+-----+-----+
     *   2  |  on | off |
     * -----+-----+-----+
     *   3  |  on |  on |
     * -----+-----+-----+
    */
    uint32_t enableB = (port == RF_B || port == RF_ALL) ? 1u : 0u;
    // R4[5] = 1 => A enabled; R4[5] = 0 => A disabled
    uint32_t enableA = (port == RF_A || port == RF_ALL) ? 1u : 0u;

    // Writing bits via setRegisterField so the dirty bits are set
    setRegisterField(4, 8, 8, enableB); // R4 bit 8
    setRegisterField(4, 5, 5, enableA); // R4 bit 5
    updateRegisters();
}

// RFOutPort = RFNONE, RF_A, RF_B or RF_ALL
void MAX2871::outputPower(int dBm, RFOutPort port) {
    // dBm:     Valid values: -4, -1, +2, +5 dBm
    // port:    Power Level - R4[7:6] sets RFoutB and R4[4:3] sets RFoutA
    uint32_t code = 0;
    switch (dBm) {
        case -4: code = 0u; break;
        case -1: code = 1u; break;
        case  2: code = 2u; break;
        case  5: code = 3u; break;
        default:
            return; // invalid value - leave power level unchanged
    }
    if (port == RF_A || port == RF_ALL) {
        setRegisterField(4, 4, 3, code); // write the Port A power level into R4[4:3]
    }
    if (port == RF_B || port == RF_ALL) {
        setRegisterField(4, 7, 6, code); // write the Port B power level into R4[7:6]
    }
    updateRegisters();
}

// ---- Status ----

bool MAX2871::isLocked() {
    return hal ? hal->readMuxout() : false;
}

// ---- Register Access ----

void MAX2871::writeRegister(uint32_t value) {
    if (hal) {
        hal->spiWriteRegister(value);
    }
}

/*  At power-up, the registers should be programmed twice. The first
 *  write ensures the device is enabled, and the second write starts
 *  the VCO selection process.
*/
void MAX2871::updateRegisters() {
    // Additional write cycle after a reset
    if (first_init) {
        writeRegister(Curr.Reg[5]);                         // Program register 5
        if (hal) hal->delayMs(20);
        writeRegister(Curr.Reg[4] & 0xFFFFFEDF);            // Disable RFOUTA and RFOUTB
        for (int regAddr = 3; regAddr >= 0; --regAddr) {    // Program registers 3, 2, 1, 0
            writeRegister(Curr.Reg[regAddr]);
        }
        // Force register writes 
        _dirtyMask = 0x3F;
    }

    if (_dirtyMask == 0) return;    // Exit early if no registers to update

    for (int regAddr = 5; regAddr >= 0; --regAddr) {
        if ((_dirtyMask & (1UL << regAddr)) != 0) {
            writeRegister(Curr.Reg[regAddr]);
        }
    }
    _dirtyMask = 0;
}

// Reset working copy of registers, Curr, from the defaultRegisters
void MAX2871::reset() {
    first_init = true;          // Flag to run the clean-clock startup once
    Curr = defaultRegisters;    // Reset all registers to their default values
    updateRegisters();          // Performs clean-clock startup sequence
    first_init = false;         // Done running clean-clock startup
}

/*  This function provides the expert with the ability to manually change the bits
    within the registers. You can stack as many calls to setRegisterField as you like
    and then make a call to updateRegisters() to program the chip with your changes.
 */
void MAX2871::setRegisterField(uint8_t regAddr, uint8_t bit_hi, uint8_t bit_lo, uint32_t value) {
    // --- Input validation ---
    // Swap bit_lo <---> bit_hi if bit_lo is higher
    if (bit_lo > bit_hi) {
        bit_lo ^= bit_hi,       // bit_lo now holds XOR of both
        bit_hi ^= bit_lo,       // bit_hi now holds original bit_lo
        bit_lo ^= bit_hi;       // bit_lo now holds original bit_hi
    }
    // bits 2:0 contain register address 6 down to 0. Bits 31:3 register data
    if (bit_lo < 3 || bit_hi > 31 || regAddr > 6) return;

    // --- Check Register values ---
    uint32_t oldReg = Curr.Reg[regAddr];                // Checking for Curr.Reg value changes
    uint32_t mask = bitMask(bit_hi, bit_lo);            // Create mask for clearing bit field
    uint32_t data = fieldValue(value, bit_hi, bit_lo);  // Create data for filling field
    uint32_t newReg = (Curr.Reg[regAddr] & ~mask) | data;
    if (newReg != oldReg) {
        Curr.Reg[regAddr] = newReg;
        _dirtyMask |= (1 << regAddr);  // selected register being marked for update
    }
    _dirtyMask |= (_dirtyMask >> 1) & 1UL;  // If reg1 is marked then mark reg0 (double buffer)
    _dirtyMask |= (_dirtyMask >> 4) & 1UL;  // If reg4 is marked then mark reg0
}
