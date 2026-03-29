/* hal.h
    Shared low-level utilities for the hardware-facing layers.

    This file intentionally does not define a controller interface anymore.
    It only provides common pin enums and bitfield helpers used by the MCU
    and transport abstractions.

    (c) 2025 Mark Stanley, GPL-3.0-or-later
 */
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#if !defined(HAL_INPUT_PULLDOWN) && defined(INPUT_PULLDOWN)
#define HAL_INPUT_PULLDOWN INPUT_PULLDOWN
#elif !defined(HAL_INPUT_PULLDOWN) && defined(INPUT)
#define HAL_INPUT_PULLDOWN INPUT
#endif

enum pin_mode { PINMODE_INPUT, PINMODE_INPUT_PULLUP, PINMODE_INPUT_PULLDOWN, PINMODE_OUTPUT };
enum pin_level { PINLEVEL_LOW = 0, PINLEVEL_HIGH = 1 };

// --- Bitfield Utilities ---
// Helper functions for register bit manipulation

// Return a mask covering bits [bit_lo : bit_hi], inclusive.
// Example: bitMask(12, 5) -> 0b00011111100000
inline uint32_t bitMask(uint8_t bit_hi, uint8_t bit_lo) {
    if (bit_hi > 31 || bit_lo > bit_hi) return 0;
    return ((0xFFFFFFFFu >> (31 - (bit_hi - bit_lo))) << bit_lo);
}

// Return 'value' aligned into field [bit_lo : bit_hi], with other bits zero.
// Example: fieldValue(0b101, 7, 5) -> 0b00000000000000000000001010000000
inline uint32_t fieldValue(uint32_t value, uint8_t bit_hi, uint8_t bit_lo) {
    if (bit_hi > 31 || bit_lo > bit_hi) return 0;
    uint32_t mask = bitMask(bit_hi, bit_lo);
    return (value << bit_lo) & mask;
}
#endif // HAL_H
