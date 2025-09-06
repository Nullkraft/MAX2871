#include <math.h>
#include "max2871.h"
#include "max2871_hal.h"

// MAX2871_LO::MAX2871_LO(double refIn) : Frac(0), M(0), N(0), DIVA(0), refInHz(refIn) {}
MAX2871::MAX2871(double refIn) : refInHz(refIn) {}

void MAX2871::freq2FMN(float target_freq_MHz) {
    float floatFrac;
    R = 1;
    Fpfd = refInHz / R;           // Phase Frequency Detector input frequency
    float max_error = pow(2, 32);       // Large initial error
    float Fvco = target_freq_MHz;

    // Adjust Fvco to be within 3000 to 6000 MHz range and calculate DIVA accordingly
    DIVA = 0;                           // Re-initialize DIVA (divide by 1)
    while (Fvco < 3000.0) {
        Fvco *= 2;                      // Double until VCO is in valid range
        DIVA += 1;                      // Track number of doublings to determine DIVA
    }

    float NdotF = Fvco / Fpfd;
    N = static_cast<uint8_t>(NdotF);    // Integer portion (N of NdotF)
    floatFrac = NdotF - N;       // Fractional portion (F of NdotF)
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
            max_error = err;            // Update minimum error
            best_F = F_candidate;       // Best fractional candidate
            best_M = M_candidate;       // Best modulus candidate
        }
    }

    Frac = static_cast<uint16_t>(round(best_F));    // Finalize fractional component
    M = best_M;                                     // Finalize modulus
}


double MAX2871::fmn2freq() {
    // Compute VCO frequency
    double fVCO = this->Fpfd * (this->N + (double)this->Frac / this->M);

    // Divide by 2^DIVA to get output frequency
    double fout = fVCO / (1 << this->DIVA);

    return fout;
}


void MAX2871::writeRegister(uint32_t value) {
    if (hal) hal->spiWriteRegister(value);
}

void MAX2871::setCE(bool enabled) {
    if (hal) hal->setCEPin(enabled);
}

bool MAX2871::isLocked() {
    return hal ? hal->readMuxout() : false;
}
