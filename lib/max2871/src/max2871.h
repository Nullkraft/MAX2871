#ifndef _MAX2871_
#define _MAX2871_

#include <stdint.h>   // fixed-width integer types like uint32_t

class MAX2871 {
  typedef struct maxRegisters {
    static constexpr uint8_t numRegisters = 7;
    uint32_t Reg[numRegisters] = {0x001D47B0,  // R[0] N = Bits[30:15], F = Bits[14:3]
                                  0x40017FE1,  // R[1] M = Bits[14:3]
                                  0x80005F42,  // R[2] Digital Lock detect OFF
                                  0x04009F23,  // R[3] Fast Lock enabled
                                  0x638E83C4,  // R[4] RFout_B enabled @ +5dBm / RFout_A disabled
                                  0x00400005,  // R[5]
                                  0x98005F42   // R[6] Digital Lock detect ON
                                };
  } max2871Registers;

public:
  MAX2871() : first_init(true) {}
  MAX2871(double refIn = 66.0);

  max2871Registers Curr;  // Read/Write copy of the registers

  // Frequency divider values
  uint32_t Frac;  // Fractional value
  uint16_t M;     // Modulus
  uint16_t N;     // Integer division
  uint8_t DIVA;   // Output divider (VCO divider setting)
  double Fpfd;
  int R;

  // Reference input frequency and phase detector freq
  double refInHz;  // Reference clock frequency
  double fpfdHz;   // fPfd = refInHz / R  (R = ref divider)

private:
  // The MAX2871 requires a 20 ms delay only on the first init
  bool first_init;

  /* 6 bit mask of Embedded Data from serial Specific Command */
  static constexpr short Data_Mask = 0x3F;

  /* 12 bit mask, R[1] bits [14:3], for Fractional Modulus Value, M */
  #define M_set 0x7FF8
  #define M_clr 0xFFFF8007

  /* 12 bit mask, R[0] bits [14:3], for Frequency Division Value, F */
  #define F_set 0x7FF8
  #define F_clr 0xFFFF8007

  /* 8 bit mask, R[0] bits [22:15], for Integer Counter, N */
  #define N_set 0x7F8000
  #define N_clr 0xFFFF8007

  /* 20 bit mask, R[0] bits [22:3], for N and F */
  #define NF_set 0x7FFFF8
  #define NF_clr 0xFF800007

  /* R4<8> and R4<5> disable RFoutB and RFoutA */
  static constexpr uint32_t RFpower_off = 0xFFFFFE07;

  /* R4<7:6> Adjust the RFoutB power level. (RFoutA is off by default) */
  static constexpr uint32_t Power_Level_Mask = RFpower_off;

  /* RFoutB power levels */
  static constexpr uint32_t neg4dBm = 0x100;
  static constexpr uint32_t neg1dBm = 0x140;
  static constexpr uint32_t pos2dBm = 0x180;
  static constexpr uint32_t pos5dBm = 0x1C0;

  /* R4<22:20> Set the RFOut Divider Mode */
  #define RFOUT_DIV_MASK 0xFF8FFFFF

  /*****  ----------------------- NOTE: --------------------------  *****/
  /***** | Enabling Tristate, Mux_Set_TRI, automatically disables | *****/
  /***** | Digital Lock Detect, Mux_Set_DLD, and vice versa.      | *****/
  /*****  --------------------------------------------------------  *****/

  static constexpr uint32_t Mux_Set_TRI = 0xE3FFFFFF;
  static constexpr uint32_t Mux_Set_DLD = 0x18000000;

  uint32_t spiMaxSpeed = 20000000;  // 20 MHz max SPI clock

public:
  // Calculate F, M, N, DIVA from input frequency (MHz)
  void freq2FMN(float target_freq_MHz);

  // Calculate actual frequency from current F, M, N, DIVA
  double fmn2freq();
};

#endif
