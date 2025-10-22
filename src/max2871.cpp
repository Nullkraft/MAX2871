#include "max2871.h"

// ---- Static read-only defaults ----
const MAX2871::max2871Registers MAX2871::defaultRegisters = {{
    0x001A8008,  // Register 0 - Contains F and parts of N
    0x40010019,  // Register 1 - Contains M
    0x98007F42,  // Register 2 - Digital Lock Detect (DLD) on
    0x00001F23,  // Register 3
    0x63EE81FC,  // Register 4
    0x00400005   // Register 5
}};

// ---- Construction ----

/* hal defaults to nullptr */
MAX2871::MAX2871(double refIn) 
    : refInHz(refIn), _lePin(0), first_init(true), _dirtyMask(0) {
}

void MAX2871::begin(uint8_t lePin) {
    _lePin = lePin;
    reset();  // Fill the working (shadow) registers
}

// ---- Frequency Control ----

void MAX2871::setFrequency(double freqMHz) {
    freq2FMN(freqMHz);
    setRegisterField(4, 22, 20, DIVA);
    setRegisterField(1, 14,  3, M);
    setRegisterField(0, 14,  3, Frac);
    setRegisterField(0, 30, 15, N);
    updateRegisters();
}

void MAX2871::setFrequency(uint32_t fmn, uint8_t diva) {
    // TODO: unpack FMN + DIVA
    // TODO: update shadow registers directly
    // TODO: program chip via writeRegister/setAllRegisters
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
    double fVCO = this->Fpfd * (this->N + (double)this->Frac / this->M);
    double fout = fVCO / (1 << this->DIVA);
    return fout;
}

// ---- Output Control ----

void MAX2871::outputSelect(uint8_t sel) {
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
    uint32_t enableB = (sel == 2 || sel == 3) ? 1u : 0u;
    // R4[5] = 1 => A enabled; R4[5] = 0 => A disabled
    uint32_t enableA = (sel == 1 || sel == 3) ? 1u : 0u;

    // Writing bits via setRegisterField so the dirty bits are set
    setRegisterField(4, 8, 8, enableB); // R4 bit 8
    setRegisterField(4, 5, 5, enableA); // R4 bit 5
}

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
    if (port == 1 || port == 3) {
        setRegisterField(4, 4, 3, code); // write the Port A power level into R4[4:3]
    }
    if (port == 2 || port == 3) {
        setRegisterField(4, 7, 6, code); // write the Port B power level into R4[7:6]
    }
    updateRegisters();
}

// ---- Mode Control ----

void MAX2871::mode(uint8_t type) {
    // TODO: set integer-N or fractional-N mode
    // type: 0=integer, 1=fractional
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

void MAX2871::setAllRegisters() {
    // Writes all shadow registers R6..R0 to the I.C.
    for (int regAddr = 6; regAddr >= 0; --regAddr) {
        writeRegister(Curr.Reg[regAddr]);
    }

    // Full clean write completed, clear the dirty mask
    _dirtyMask = 0;
}


// Only the registers flagged in the _dirtyMask will be programmed.
void MAX2871::updateRegisters() {
    if (first_init) {
        // Clean-clock startup sequence. Runs once after a reset
        writeRegister(Curr.Reg[5]);                                 // Program register 5
        if (hal) hal->delayMs(20);
        uint32_t r4_temp = Curr.Reg[4] & ~((1u << 8) | (1u << 5));  // Disable RFOUTA and RFOUTB
        writeRegister(r4_temp);                                     // Program register 4

        for (int regAddr = 3; regAddr >= 0; --regAddr) {                        // Program registers 3, 2, 1, 0
            writeRegister(Curr.Reg[regAddr]);
        }
        // Force a second programming cycle to registers 0-5
        _dirtyMask = 0x3F;
    }

    // Early exit if nothing to update
    if (_dirtyMask == 0) return;

    _dirtyMask |= (_dirtyMask >> 1) & 1UL;  // Copy Reg1 dirty bit to Reg0

    // If DIVA changed in register 4 then mark Reg0 as dirty
    uint8_t currDIVA = (Curr.Reg[4] >> 20) & 0x7;
    if (currDIVA != _lastDIVA) {
        _dirtyMask |= 1;    // Mark Reg0 as dirty
    }
    _lastDIVA = (Curr.Reg[4] >> 20) & 0x7;                          // Update _lastDIVA to new value

    // Update dirty registers
    for (int regAddr = 5; regAddr >= 0; --regAddr) {
        if ((_dirtyMask & (1UL << regAddr)) != 0) {
            writeRegister(Curr.Reg[regAddr]);
            _dirtyMask = (uint8_t)(_dirtyMask & ~(1UL << regAddr));     // Clear dirty bit on current register?
        }
    }
}

// Reset working copy of registers, Curr, from the defaultRegisters
void MAX2871::reset() {
    first_init = true;          // Flag to run the clean-clock startup once
    Curr = defaultRegisters;    // Reset all registers to their default values
    updateRegisters();          // Performs clean-clock startup sequence
    first_init = false;         // Done running clean-clock startup
}

/*  This function provides the expert with the ability to manually change the bits
    within the registers. You can stack as many calls to setRegisterField as you 
    like and then make a final call to updateRegisters() to program the chip with
    your changes.
 */
void MAX2871::setRegisterField(uint8_t regAddr, uint8_t bit_hi, uint8_t bit_lo, uint32_t value) {
    // Swap bit_lo and bit_hi if bit_lo is higher than bit_hi
    if (bit_lo > bit_hi) {
        uint8_t bit_temp = bit_hi;
        bit_hi = bit_lo;
        bit_lo = bit_temp;
    }

    /* --- Input validation ---
     * bit_lo  : 3 or higher (2:0 reserved for register address)
     * bit_hi  : 31 or lower (registers are 32 bits)
     * regAddr : 7 registers (0 to 6)
    */
    if (bit_lo < 3 || bit_hi > 31 || regAddr > 6) return;

    uint32_t oldVal = Curr.Reg[regAddr];

    // Clear oldVal bit range before inserting (ANDing) new values
    uint32_t mask = bitMask(bit_hi, bit_lo);

    // TODO: Add mask as an input to fieldValue() to eliminate
    //       the internal call to bitMask() from fieldValue()
    // Insert new value into the target bitfield
    uint32_t newVal = (Curr.Reg[regAddr] & ~mask) | fieldValue(value, bit_hi, bit_lo);

    // Only write if something actually changed
    if (newVal != oldVal) {
        Curr.Reg[regAddr] = newVal;
        _dirtyMask |= (1 << regAddr);  // mark this register for update
    }
}

// ---- Private Helpers ----

void MAX2871::spiWrite(uint32_t value) {
    // TODO: split 32-bit into 4 bytes, call hal->spiTransfer() for each
    // then toggle selectChip()
}
