# MAX2871 PLL Synthesizer Library

A hardware abstraction library for controlling the MAX2871 wideband PLL synthesizer (23.5-6000 MHz) on Arduino and compatible platforms.

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

BitBangHAL hal(PIN_CLK, PIN_DAT, PIN_LE);
MAX2871 lo(REF_MHZ);

void outputEnable(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

void setup() {
    hal.begin();
    lo.attachHal(&hal);
    lo.begin(PIN_LE);
    outputEnable(RF_EN);
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