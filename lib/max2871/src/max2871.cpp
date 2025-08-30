#include "max2871.h"

MAX2871_LO::MAX2871_LO(double refIn) : Frac(0), M(0), N(0), DIVA(0), refInHz(refIn) {}

void MAX2871_LO::freq2FMN(uint64_t target_freq_MHz) {
    float R = 1;
    float Fpfd = this->refInHz / R;                // Phase Frequency Detector input frequency
    float max_error = pow(2, 32);              // Large initial error
    float Fvco = target_freq_MHz;

    // Adjust Fvco to be within 3000 to 6000 MHz range and calculate DIVA accordingly
    DIVA = 0;                                   // Re-initialize DIVA (divide by 1)
    while (Fvco < 3000.0) {
        Fvco *= 2;                              // Double until VCO is in valid range
        DIVA += 1;                              // Track number of doublings to determine DIVA
    }

    float NdotF = Fvco / Fpfd;
    this->N = static_cast<uint8_t>(NdotF);      // Integer portion (N of NdotF)
    this->Frac = NdotF - this->N;               // Fractional portion (F of NdotF)

    float best_F = 0;
    uint16_t best_M = 0;

    // Loop through M from 4095 down to 2
    for (int M_candidate = 4095; M_candidate > 1; --M_candidate) {
        float F_candidate = this->Frac * M_candidate;
        float FvcoCalculated = Fpfd * (this->N + F_candidate / M_candidate);
        float err = abs(Fvco - FvcoCalculated);
        if (err == 0) {
            best_F = F_candidate;              // Perfect match found
            best_M = M_candidate;
            break;
        }
        if (err <= max_error) {
            max_error = err;                   // Update minimum error
            best_F = F_candidate;              // Best fractional candidate
            best_M = M_candidate;              // Best modulus candidate
        }
    }

    this->Frac = static_cast<uint16_t>(round(best_F));  // Finalize fractional component
    this->M = best_M;                                // Finalize modulus
}
