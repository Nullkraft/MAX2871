#ifndef _MAX2871_
#define _MAX2871_

#include <math.h>
#include "I_PLLSynthesizer.h"   // Common PLL interface (#includes HAL)

class MAX2871 : public I_PLLSynthesizer {
public:
  struct max2871Registers {
    static constexpr uint8_t numRegisters = 7;
    uint32_t Reg[numRegisters];
  };

  // ---- JUNK Delete Me When Done ----
  uint32_t print_val1 = 0;
  uint32_t print_val2 = 0;

  // public members
  HAL* hal = nullptr;

  // ---- Construction / Init ----
  explicit MAX2871(double refIn);
  MAX2871() = delete;                                       // Disallow empty constructor

  void attachHal(HAL* halPtr) override {hal = halPtr;}
  void begin() override;

  // ---- Frequency Control ----
  void setFrequency(double freqMHz) override;               // calculates FMN+DIVA
  void setFrequency(uint32_t fmn, uint8_t diva) override;   // bypass math
  void freq2FMN(float target_freq_MHz);                     // calculate F,M,N,DIVA
  double fmn2freq();                                        // reverse calc

  // ---- Output Control ----
  void outputSelect(RFOutPort port) override;   // A, B, both, or off
  void outputPower(int dBm, RFOutPort port) override;        // -4, -1, +2, +5 dBm

  // ---- Mode Control ----
  void mode(uint8_t type) override;          // 0=int-N, 1=frac-N

  // ---- Status ----
  bool isLocked() override;

  // ---- Register Access ----
  void writeRegister(uint32_t value);
  void setAllRegisters();
  void updateRegisters();
  void reset();
  void setRegisterField(uint8_t reg, uint8_t bit_hi, uint8_t bit_lo, uint32_t value);
  uint8_t getDirtyMask() const { return _dirtyMask; }
  void clearDirtyMask() { _dirtyMask = 0; }
  inline void markDirty(uint8_t reg) { _dirtyMask |= static_cast<uint8_t>(1UL << reg); } // compiler truncates to 8 bits

  // ---- State ----
  // Read-only default registers (for dev and reset() only)
  static const max2871Registers defaultRegisters;
  // Working copy of registers (mutable shadow registers)
  max2871Registers Curr;

  // Frequency divider values
  uint32_t Frac;
  uint16_t M;
  uint16_t N;
  uint8_t DIVA;
  double Fpfd;
  int R;

private:
  // control
  double refInHz;                   // Reference clock input frequency
  double fpfdHz;                    // phase detector frequency
  uint8_t _rfEnPin;
  bool first_init;
  uint8_t _dirtyMask = 0;
  uint8_t _lastDIVA = 0xFF;         // Should reg4 be double buffered (DIVA changed or not) by reg1?
  uint32_t spiMaxSpeed = 20000000;  // 20 MHz
};

#endif
