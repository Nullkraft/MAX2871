#ifndef _MAX2871_
#define _MAX2871_

#include <math.h>
#include "I_PLLSynthesizer.h"   // Common PLL interface
#include "mcu_hal.h"
#include "max2871_transport.h"

class MAX2871 : public I_PLLSynthesizer {
public:
  struct max2871Registers {
    static constexpr uint8_t numRegisters = 7;
    uint32_t Reg[numRegisters];
  };

  // explicit MAX2871(double refIn);
  explicit MAX2871(uint32_t refKHz, I_MAX2871Transport& transport, IDelayProvider& timing);
  MAX2871(uint32_t refKHz, I_MAX2871Transport& transport, IDelayProvider& timing,
          const max2871Registers& startupRegisters);
  MAX2871() = delete;                                       // Disallow empty constructor
  void begin() override;
  void reset();
  bool isLocked() override;

  // ---- Frequency Control ----
  void setFrequency(uint32_t freqKHz) override;             // calculates FMN+DIVA
  bool setFrequency(uint16_t frac, uint16_t modulus, uint16_t n, uint8_t diva);
  bool freq2FMN(uint32_t targetFreqKHz);                    // calculate F,M,N,DIVA
  uint32_t fmn2freq();                                      // reverse calc in kHz

  // ---- Output Control ----
  void outputSelect(RFOutPort port = RF_ALL) override;          // A, B, both, or off
  void outputPower(int dBm, RFOutPort port = RF_ALL) override;  // -4, -1, +2, +5 dBm

  // Default registers - Read-only
  static const max2871Registers defaultRegisters;
  // Working registers - Read/Write
  max2871Registers Curr;

  // Frequency divider values
  uint16_t Frac;
  uint16_t M;
  uint16_t N;
  uint8_t DIVA;
  uint32_t Fpfd;
  int R;

private:
  uint32_t _refKHz;                 // Reference clock input frequency
  I_MAX2871Transport& _transport;
  IDelayProvider& _timing;
  max2871Registers _startupRegisters;
  bool first_init;
  uint8_t _dirtyMask;               // Track which registers require programming

  void writeRegister(uint32_t value);
  void updateRegisters();
  void setRegisterField(uint8_t reg, uint8_t bit_hi, uint8_t bit_lo, uint32_t value);
};

#endif
