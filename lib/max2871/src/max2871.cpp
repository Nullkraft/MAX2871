#include "max2871.h"

// ---- Construction ----

MAX2871::MAX2871(double refIn) 
    : refInHz(refIn), hal(nullptr), _lePin(0), first_init(true), _dirtyMask(0) {
    // Initialize shadow registers to defaults
    for (int i = 0; i < Curr.numRegisters; i++) {
        Curr.Reg[i] = Curr.Reg[i];  // already initialized in struct
    }
}

void MAX2871::begin(uint8_t lePin) {
    _lePin = lePin;
    if (hal) {
        hal->pinMode(_lePin, PINMODE_OUTPUT);
        hal->digitalWrite(_lePin, PINLEVEL_HIGH); // idle high
    }
}

void MAX2871::attachHal(HAL* halptr) {
    hal = halptr;
}

// ---- Frequency Control ----

void MAX2871::setFrequency(double freqMHz) {
    // TODO: call freq2FMN() to calculate F, M, N, DIVA
    // TODO: update shadow registers with those values
    // TODO: program chip via writeRegister/setAllRegisters
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
    // TODO: modify R[4] bits to enable A/B outputs
    // sel: 0=off, 1=A, 2=B, 3=both
}

void MAX2871::outputPower(int dBm) {
    // TODO: adjust R[4] power level bits
    // Valid values: -4, -1, +2, +5 dBm
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
    for (int reg = 6; reg >= 0; --reg) {
        writeRegister(Curr.Reg[reg]);
    }

    // Full clean write completed, clear the dirty mask
    _dirtyMask = 0;
}

void MAX2871::updateRegisters() {
    if (_dirtyMask == 0) return;

    uint8_t mask = _dirtyMask;

    // If R4 is dirty, force-write R0 as well (without setting its dirty bit).
    bool forceR0 = (mask & (1UL << 4)) != 0;

    for (int reg = 6; reg >= 0; --reg) {
        bool shouldWrite = (mask & (1UL << reg)) != 0;

        // If R0 wasn’t dirty but R4 was, include R0
        if (!shouldWrite && forceR0 && reg == 0) {
            shouldWrite = true;
        }

        if (shouldWrite) {
            writeRegister(Curr.Reg[reg]);
            // Clear only the bit we actually wrote (don’t clear R0 if it wasn’t set)
            if ((mask & (1UL << reg)) != 0) {
                _dirtyMask = (uint8_t)(_dirtyMask & ~(1UL << reg));
            }
        }
    }
}

void MAX2871::setRegisterField(uint8_t reg, uint8_t bit_hi, uint8_t bit_lo, uint32_t value) {
    // Clamp field so we never touch the register address bits [2:0].
    if (bit_lo < 3) bit_lo = 3;

    // --- Input validation ---
    if (reg > 6 || bit_hi > 31 || bit_lo > bit_hi) return;

    // Preserve register address (3 LSBs)
    const uint32_t REG_ADDR_MASK = 0x7; // bits [2:0]
    uint32_t oldVal = Curr.Reg[reg];

    uint32_t mask = bitMask(bit_hi, bit_lo);

    // Insert new value into the target bitfield
    uint32_t newVal = (oldVal & ~mask) | fieldValue(value, bit_hi, bit_lo);

    // Preserve the register address bits
    newVal = (newVal & ~REG_ADDR_MASK) | (oldVal & REG_ADDR_MASK);

    // Only write if something actually changed
    if (newVal != oldVal) {
        Curr.Reg[reg] = newVal;
        _dirtyMask |= (1 << reg);  // mark this register for update
    }
}


// ---- Private Helpers ----

void MAX2871::spiWrite(uint32_t value) {
    // TODO: split 32-bit into 4 bytes, call hal->spiTransfer() for each
    // then toggle latch()
}

void MAX2871::latch() {
    if (hal) {
        hal->digitalWrite(_lePin, PINLEVEL_LOW);
        hal->digitalWrite(_lePin, PINLEVEL_HIGH);
    }
}
