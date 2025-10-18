#ifndef _MAX2871_
#define _MAX2871_

#include <math.h>
#include "I_PLLSynthesizer.h"   // Common PLL interface

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
  void begin(uint8_t lePin) override;                       // new: init with LE pin

  // ---- Frequency Control ----
  void setFrequency(double freqMHz) override;               // calculates FMN+DIVA
  void setFrequency(uint32_t fmn, uint8_t diva) override;   // bypass math
  void freq2FMN(float target_freq_MHz);                     // calculate F,M,N,DIVA
  double fmn2freq();                                        // reverse calc

  // ---- Output Control ----
  void outputSelect(uint8_t sel) override;   // A, B, both, or off
  void outputPower(int dBm) override;        // -4, -1, +2, +5 dBm
  void outputEnable(uint8_t rfEn) override;  // Enable RF output

  // ---- Mode Control ----
  void mode(uint8_t type) override;          // 0=int-N, 1=frac-N

  // ---- Status ----
  bool isLocked() override;

  // ---- Register Access ----
  void writeRegister(uint32_t value);
  void setAllRegisters();
  void updateRegisters();
  void resetToDefaultRegisters();
  void setRegisterField(uint8_t reg, uint8_t bit_hi, uint8_t bit_lo, uint32_t value);
  uint32_t getRegister(uint8_t reg) const { return Curr.Reg[reg]; }
  uint8_t getDirtyMask() const { return _dirtyMask; }
  void clearDirtyMask() { _dirtyMask = 0; }
  inline void markDirty(uint8_t reg) { _dirtyMask |= static_cast<uint8_t>(1UL << reg); } // compiler truncates to 8 bits

  // --- Bitfield helpers ---

  // Return a mask covering bits [bit_lo : bit_hi], inclusive.
  // Example: bitMask(12, 5) -> 0b00011111100000
  static inline uint32_t bitMask(uint8_t bit_hi, uint8_t bit_lo) {
      if (bit_hi > 31 || bit_lo > bit_hi) return 0;
      return ((0xFFFFFFFFu >> (31 - (bit_hi - bit_lo))) << bit_lo);
  }

  // Return 'value' aligned into field [bit_lo : bit_hi], with other bits zero.
  // Example: fieldValue(0b101, 7, 5) -> 0b00000000000000000000001010000000
  static inline uint32_t fieldValue(uint32_t value, uint8_t bit_hi, uint8_t bit_lo) {
      if (bit_hi > 31 || bit_lo > bit_hi) return 0;
      uint32_t mask = bitMask(bit_hi, bit_lo);
      return (value << bit_lo) & mask;
  }

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
  uint8_t _lePin;                   // Per-device chip select pin
  uint8_t _rfEnPin;
  bool first_init;
  uint8_t _dirtyMask = 0;
  uint32_t spiMaxSpeed = 20000000;  // 20 MHz

  // Private helpers
  void spiWrite(uint32_t value);
  void selectChip();
};

#endif
