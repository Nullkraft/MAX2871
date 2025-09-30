#ifndef _MAX2871_
#define _MAX2871_

#include <math.h>
#include "I_PLLSynthesizer.h"   // Common PLL interface

class MAX2871 : public I_PLLSynthesizer {
  typedef struct maxRegisters {
    static constexpr uint8_t numRegisters = 7;
    uint32_t Reg[numRegisters] = {
      0x001D47B0,  // R[0] N = Bits[30:15], F = Bits[14:3]
      0x40017FE1,  // R[1] M = Bits[14:3]
      0x80005F42,  // R[2] Digital Lock detect OFF
      0x04009F23,  // R[3] Fast Lock enabled
      0x638E83C4,  // R[4] RFout_B enabled @ +5dBm / RFout_A disabled
      0x00400005,  // R[5]
      0x98005F42   // R[6] Digital Lock detect ON
    };
  } max2871Registers;

public:
  // ---- Construction / Init ----
  MAX2871() : first_init(true) {}   // MAX2871 requires 2 initial programming cycles
  MAX2871(double refIn);

  void attachHal(HAL* halPtr) override;
  void begin(uint8_t lePin) override;   // new: init with LE pin

  // ---- Frequency Control ----
  void setFrequency(double freqMHz) override;              // calculates FMN+DIVA
  void setFrequency(uint32_t fmn, uint8_t diva) override;  // bypass math
  void freq2FMN(float target_freq_MHz);                    // calculate F,M,N,DIVA
  double fmn2freq();                                       // reverse calc

  // ---- Output Control ----
  void outputSelect(uint8_t sel) override;   // A, B, both, or off
  void outputPower(int dBm) override;        // -4, -1, +2, +5 dBm

  // ---- Mode Control ----
  void mode(uint8_t type) override;          // 0=int-N, 1=frac-N

  // ---- Status ----
  bool isLocked() override;

  // ---- Register Access ----
  void writeRegister(uint32_t value);
  void setAllRegisters();
  void updateRegisters();
  void setRegisterField(uint8_t reg, uint8_t bit_hi, uint8_t bit_lo, uint32_t value);
  uint32_t getRegister(uint8_t reg) const { return Curr.Reg[reg]; }
  uint8_t getDirtyMask() const { return _dirtyMask; }
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
  max2871Registers Curr;  // Shadow registers

  // Frequency divider values
  uint32_t Frac;  
  uint16_t M;
  uint16_t N;
  uint8_t DIVA;
  double Fpfd;
  int R;

  // Reference clock input frequency and phase detector frequency
  double refInHz;
  double fpfdHz;

private:
  // HAL + control
  HAL* hal = nullptr;
  uint8_t _lePin;
  bool first_init;
  uint8_t _dirtyMask = 0;

  // 6 bits of Embedded Data from serial Specific Command
  static constexpr short Data_Mask = 0x3F;
  
  // 12 bits R[1] bits [14:3] for Modulus values, M
  #define M_set 0x7FF8
  #define M_clr 0xFFFF8007
  
  // 12 bits R[0] bits [14:3] for Frequency division value, F
  #define F_set 0x7FF8
  #define F_clr 0xFFFF8007
  
  // 8 bits R[0] bits [22:15] for Integer counter, N
  #define N_set 0x7F8000
  #define N_clr 0xFFFF8007
  
  // 20 bits R[0] bits [22:3] for combining N and F
  #define NF_set 0x7FFFF8
  #define NF_clr 0xFF800007
  
  // R4[8] and R4[5] disable RFoutB and RFoutA
  static constexpr uint32_t RFpower_off = 0xFFFFFE07;

  // R4[7:6] Adjust the RFoutB power level. (RFoutA is off by default)
  static constexpr uint32_t Power_Level_Mask = RFpower_off;

  // RFoutB power levels
  static constexpr uint32_t neg4dBm = 0x100;
  static constexpr uint32_t neg1dBm = 0x140;
  static constexpr uint32_t pos2dBm = 0x180;
  static constexpr uint32_t pos5dBm = 0x1C0;

  // R4[22:20] Set the RFOut Divider Mode
  #define RFOUT_DIV_MASK 0xFF8FFFFF
  
  
  /*****  ----------------------- NOTE: --------------------------  *****/
  /***** | Enabling Tristate, Mux_Set_TRI, automatically disables | *****/
  /***** | Digital Lock Detect, Mux_Set_DLD, and vice versa.      | *****/
  /*****  --------------------------------------------------------  *****/

  static constexpr uint32_t Mux_Set_TRI = 0xE3FFFFFF;
  static constexpr uint32_t Mux_Set_DLD = 0x18000000;

  uint32_t spiMaxSpeed = 20000000;  // 20 MHz

  // Private helpers
  void spiWrite(uint32_t value);
  void latch();
};

#endif
