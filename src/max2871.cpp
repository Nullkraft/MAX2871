#include "max2871.h"
#include "hal.h"

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
// First use of _dirtyMask marks all 6 registers as requiring updates
MAX2871::MAX2871(uint32_t refKHz, I_MAX2871Transport& transport, IDelayProvider& timing)
    : _refKHz(refKHz),
      _transport(transport),
      _timing(timing),
      _startupRegisters(defaultRegisters),
      first_init(true),
      _dirtyMask(0x3F) {
}

MAX2871::MAX2871(uint32_t refKHz, I_MAX2871Transport& transport, IDelayProvider& timing,
                 const max2871Registers& startupRegisters)
    : _refKHz(refKHz),
      _transport(transport),
      _timing(timing),
      _startupRegisters(startupRegisters),
      first_init(true),
      _dirtyMask(0x3F) {
}

void MAX2871::begin() {
    reset();  // Fill the working (shadow) registers
}

// ---- Frequency Control ----

void MAX2871::setFrequency(uint32_t freqKHz) {
    if (!freq2FMN(freqKHz)) return;
    setRegisterField(1, 14,  3, M);
    setRegisterField(0, 14,  3, Frac);
    setRegisterField(0, 30, 15, N);
    setRegisterField(4, 22, 20, DIVA);
    updateRegisters();
}

bool MAX2871::setFrequency(uint16_t frac, uint16_t modulus, uint16_t n, uint8_t diva) {
    const bool integerN = (_startupRegisters.Reg[0] & 0x80000000UL) != 0;
    const uint32_t maxPfdKHz = integerN ? 140000UL : 125000UL;
    if (_refKHz < 10000 || _refKHz > maxPfdKHz ||
        modulus < 2 || modulus > 4095 || frac >= modulus || diva > 7 ||
        (integerN && (frac != 0 || n < 16)) ||
        (!integerN && (n < 19 || n > 4091))) return false;

    R = 1;
    Fpfd = _refKHz;
    Frac = frac;
    M = modulus;
    N = n;
    DIVA = diva;
    setRegisterField(1, 14,  3, M);
    setRegisterField(0, 14,  3, Frac);
    setRegisterField(0, 30, 15, N);
    setRegisterField(4, 22, 20, DIVA);
    updateRegisters();
    return true;
}

bool MAX2871::freq2FMN(uint32_t targetFreqKHz) {
    const bool integerN = (_startupRegisters.Reg[0] & 0x80000000UL) != 0;
    const uint32_t maxPfdKHz = integerN ? 140000UL : 125000UL;
    if (targetFreqKHz < 23500 || targetFreqKHz > 6000000 ||
        _refKHz < 10000 || _refKHz > maxPfdKHz) return false;

    uint32_t FvcoKHz = targetFreqKHz;
    uint8_t calculatedDiva = 0;

    // Adjust Fvco to be within 3000 to 6000 MHz range and calculate DIVA accordingly
    while (FvcoKHz < 3000000) {
        FvcoKHz *= 2;                   // Double until VCO is in valid range
        ++calculatedDiva;               // Track doublings to determine DIVA
    }

    uint32_t calculatedN = FvcoKHz / _refKHz;
    uint32_t euclidNumerator = FvcoKHz % _refKHz;
    if (integerN) {
        if (euclidNumerator != 0 || calculatedN < 16 || calculatedN > 65535) return false;
        R = 1;
        Fpfd = _refKHz;
        Frac = 0;
        M = 2;
        N = static_cast<uint16_t>(calculatedN);
        DIVA = calculatedDiva;
        return true;
    }

    uint32_t euclidDenominator = _refKHz;
    uint32_t f0 = 0;
    uint32_t m0 = 1;
    uint32_t f1 = 1;
    uint32_t m1 = 0;
    while (euclidDenominator != 0) {
        const uint32_t coefficient = euclidNumerator / euclidDenominator;
        const uint32_t remainder = euclidNumerator % euclidDenominator;
        // Flip all the fractions
        const uint32_t m2 = m0 + coefficient * m1;
        if (m2 > 4095) {
            break;
        }
        const uint32_t f2 = f0 + coefficient * f1;
        f0 = f1;
        m0 = m1;
        f1 = f2;
        m1 = m2;
        euclidNumerator = euclidDenominator;
        euclidDenominator = remainder;
    }

    uint32_t bestF;
    uint32_t bestM;
    if (euclidDenominator == 0) {
        bestF = f1;
        bestM = m1;
    } else {
        const uint32_t multiplier = (4095 - m0) / m1;
        const uint32_t f2 = f0 + multiplier * f1;
        const uint32_t m2 = m0 + multiplier * m1;
        const uint32_t convergentDifference = euclidDenominator;
        const uint32_t semiconvergentDifference =
            euclidNumerator - multiplier * euclidDenominator;
        if (convergentDifference * m2 <= semiconvergentDifference * m1) {
            bestF = f1;
            bestM = m1;
        } else {
            bestF = f2;
            bestM = m2;
        }
    }

    if (bestM == 1) {
        if (bestF == 0) {
            bestM = 2;
        } else if (calculatedN < 4091) {
            ++calculatedN;
            bestF = 0;
            bestM = 2;
        } else {
            return false;
        }
    }

    if (calculatedN < 19 || calculatedN > 4091 ||
        bestM < 2 || bestM > 4095 || bestF >= bestM) return false;

    R = 1;
    Fpfd = _refKHz;
    Frac = static_cast<uint16_t>(bestF);
    M = static_cast<uint16_t>(bestM);
    N = static_cast<uint16_t>(calculatedN);
    DIVA = calculatedDiva;
    return true;
}

uint32_t MAX2871::fmn2freq() {
    const uint32_t outputDivider = 1UL << DIVA;
    const uint32_t integerVcoKHz = Fpfd * N;
    const uint32_t integerOutputKHz = integerVcoKHz / outputDivider;
    const uint32_t remainder = integerVcoKHz % outputDivider;
    const uint32_t denominator = static_cast<uint32_t>(M) * outputDivider;
    const uint32_t fractionalNumerator = remainder * M + Fpfd * Frac;
    return integerOutputKHz + (fractionalNumerator + denominator / 2) / denominator;
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
    return _transport.readMuxout();
}

// ---- Register Access ----

void MAX2871::writeRegister(uint32_t value) {
    _transport.spiWriteRegister(value);
}

/*  At power-up, the registers should be programmed twice. The first
 *  write ensures the device is enabled, and the second write starts
 *  the VCO selection process.
*/
void MAX2871::updateRegisters() {
    // First cycle ensures a clean-clock startup
    if (first_init) {
        writeRegister(Curr.Reg[5]);                         // Program reg 5
        _timing.delayMs(20);
        writeRegister(Curr.Reg[4] & 0xFFFFFEDF);            // Program reg 4, RFOUTA and B disabled
        for (int regAddr = 3; regAddr >= 0; --regAddr) {    // Program reg 3, 2, 1, 0
            writeRegister(Curr.Reg[regAddr]);
        }
        _dirtyMask = 0x3F;                                  // 6 Registers marked for second cycle
    }

    // Second/Normal cycle programs only the registers that have changed
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
    Curr = _startupRegisters;   // Reset all registers to this instance's startup values
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
    // Return early if stepping on address bits, exceeding 32 bits or the address range
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
