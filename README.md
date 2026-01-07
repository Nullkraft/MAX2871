# MAX2871 PLL Synthesizer Library

A hardware abstraction library for controlling the MAX2871 wideband PLL synthesizer (23.5-6000 MHz) on Arduino and compatible platforms.

## Design Choices

Arduino pins:
* PLL_MUX = A2
* SEL_LO1 = A3
* SEL_LO3 = A4
* SEL_ATTEN = A5
* Serial Port Pins (reserved - not defined in source) = 0 and 1
* SEL_LO2 = 3
* PWR_DETECT = 6 (Read-only - detects +3.3V rail on RF board)
* REF_EN1 = 8
* REF_EN2 = 9
* SPI_MOSI/SPI_MISO/SPI_CLK (reserved - not defined in source) = 11, 12, and 13

- Pins are unchangeable but defined in the sketch so they can be seen
- The SA library and the comms library will be imported into the sketch.
- There are 2 circuit boards, the Arduino and the RF board.
- The RF board is nothing without the Arduino. It defines the Spectrum Analyzer by coordinating the control of the components on the RF board.

The 2 reference clocks are selected by REF_EN1 and REF_EN2. The selection process for which clock to use will be driven by data from the PC.

Software Stack:
+----------------------------------------------------------------------------------+
| Arduino sketch (application)                                                     |
|   - includes only SpecAnn.h                                                      |
|   - calls sa.begin(), sa.setRFin(), sa.readAmplitude(), etc.                     |
|----------------------------------------------------------------------------------|
| SpecAnn hardware module (composition / orchestration)                            |
|   - owns LO1/LO2/LO3 objects via I_PLLSynthesizer                                |
|   - owns attenuator, ADC, ref enables, mode selection                            |
|   - implements “instrument behaviors” (sweep, calibration states, etc.)          |
|----------------------------------------------------------------------------------|
| Device interfaces (capabilities)                                                 |
|   - I_PLLSynthesizer (setFrequency(FMN), outputSelect, outputPower, isLocked)    |
|   - I_Attenuator, I_ADC, I_Memory (optional future)                              |
|----------------------------------------------------------------------------------|
| Chip drivers (implement device interfaces)                                       |
|   - MAX2871 : I_PLLSynthesizer                                                   |
|   - ADF4356 : I_PLLSynthesizer (future)                                          |
|----------------------------------------------------------------------------------|
| Board IO HAL (electrical primitives)                                             |
|   - HAL (spiWriteRegister, digitalWrite, pinMode, delay, readMuxout, etc.)       |
|   - FeatherHAL/ArduinoHAL implement HAL                                          |
+----------------------------------------------------------------------------------+

## License

This project is licensed under the **GPL-3.0-or-later**. See the LICENSE file for details.

Copyright (c) 2025 Mark Stanley

## Overview

This library provides a clean, Arduino-friendly API for programming the MAX2871 frequency synthesizer chip. It includes:

- **Hardware abstraction layer (HAL)** - Eval board uses IO pins but driver also supports hardware SPI
- **Simple frequency control** - just call `setFrequency(freq_MHz)`
- **Output power control** - -4, -1, +2, +5 dBm
- **PlatformIO project structure** - ready for testing and deployment

## Platform

This is a **PlatformIO** project. While the library is Arduino-compatible, it uses PlatformIO's build system for development, testing, and deployment.

> **Arduino IDE**
>
> The repository layout now follows the standard Arduino library structure (`library.properties`, `src/`, `examples/`).
> Clone or copy the folder into your Arduino `libraries/` directory and open any sketch under `examples/` directly inside the Arduino IDE without moving files. PlatformIO users can continue to work from the same tree with no changes.

### Requirements

- [PlatformIO Core](https://platformio.org/) or PlatformIO IDE
- Arduino Uno (or compatible AVR board)
- [MAX2871 evaluation board](https://github.com/Nullkraft/6GHz-Signal-Generator)

## Quick Start - "Blinky" for MAX2871

The synthesizer equivalent of the classic LED blink - generate a 42 MHz signal:
```cpp
#include <Arduino.h>
#include "max2871.h"
#include "bitbang_hal.h"

// Pin definitions
static constexpr uint8_t RF_EN   = 5;       // RF output enable
static constexpr uint8_t PIN_LE  = A3;      // Latch enable
static constexpr uint8_t PIN_DAT = A2;      // Data
static constexpr uint8_t PIN_CLK = A1;      // Clock
static constexpr double  REF_MHZ = 60.0;    // Reference clock

BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE, RF_EN);
MAX2871 lo(REF_MHZ);

void setup() {
    hal.begin();
    lo.attachHal(&hal);
    lo.begin(PIN_LE);
    hal.setCEPin(true);
    lo.setFrequency(42.0);   // Set RFOut to 42 MHz
}

void loop() {
    // Your application code here
}
```

Connect an oscilloscope or frequency counter to the RF output and verify 42 MHz!

## Building and Uploading

### Build for Arduino Uno
```bash
pio run -e uno
```

### Upload to board
```bash
pio run -e uno --target upload
```

### Run tests on hardware
```bash
pio test -e eval_board
```

### Run PC-native tests
```bash
pio test -e native
```

## API Reference

### Basic Frequency Control
```cpp
lo.setFrequency(2400.0);        // Set to 2.4 GHz
```

### Output Control
```cpp
lo.outputSelect(3);             // 0=off, 1=A only, 2=B only, 3=both
lo.outputPower(5);              // -4, -1, +2, or +5 dBm
```

### Status
```cpp
bool locked = lo.isLocked();    // Check PLL lock status
```

## Hardware Abstraction Layer (HAL)

The library supports multiple communication methods:

- **BitBangHAL** - Software SPI using GPIO pins (included)
- **ArduinoHAL** - Hardware SPI peripheral (future)
- **MockHAL** - Testing on PC without hardware (future)

All HALs implement the same interface, so switching is transparent to your application code.

## Testing

The project includes comprehensive tests using Unity test framework:

- **PC-native tests** - Run on your development machine
- **Hardware tests** - Run on actual Arduino hardware
- **Evaluation board tests** - Specific to MAX2871 eval board

## Contributing

Contributions welcome! Please ensure:
- Code follows existing style
- All tests pass (`pio test`)
- New features include tests

## References

- [MAX2871 Datasheet](https://www.analog.com/en/products/max2871.html)
- [PlatformIO Documentation](https://docs.platformio.org/)

## Author

Mark Stanley

## Acknowledgments

Built with PlatformIO and the Unity test framework.
