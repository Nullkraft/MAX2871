# MAX2871 Library Design

## Purpose

This project is an Arduino-compatible driver library for the MAX2871 wideband PLL synthesizer. The implementation is organized so that:

- application code talks to a small, chip-agnostic synthesizer interface
- the MAX2871 driver owns all register math and programming rules
- hardware access is isolated behind a chip transport interface so the same driver can run on Arduino hardware, native tests, and smoke builds

The current library is not a full instrument stack. It is the synthesizer-driver layer that a larger RF project can compose into a signal generator or spectrum analyzer.

## Core design decisions

- Public frequency units are MHz.
- The driver accepts a reference clock frequency at construction time.
- The chip driver is stateful and keeps a shadow copy of device registers.
- Register writes are minimized by tracking dirty registers.
- The first programming cycle follows a special startup sequence instead of a generic write-all.
- Hardware-specific SPI, GPIO, delays, CE, and MUXOUT reads are abstracted behind board support code, while the driver itself depends only on a MAX2871 transport interface.
- The main user-facing class also implements `I_PLLSynthesizer` so other PLL chips can later share the same application-level interface.

## Repository shape

Recreate the project with this structure:

- `src/I_PLLSynthesizer.h`
  Common synthesizer interface.
- `src/hal.h`
  Shared pin enums and bitfield helpers.
- `src/mcu_hal.h`
  Pure MCU abstraction for GPIO, delay, and SPI primitives.
- `src/max2871_transport.h`
  MAX2871-specific transport contract.
- `src/max2871.h`
  MAX2871 class declaration and register/frequency state.
- `src/max2871.cpp`
  MAX2871 implementation.
- `src/arduino_hal.h`
  Real Arduino SPI/GPIO implementation.
- `src/mock_hal.h`
  Test double that records register writes.
- `src/smoke_hal.h`
  Minimal no-op HAL for build smoke tests.
- `src/main_entry.cpp`
  Stub entrypoint so PlatformIO can link library and test targets.
- `examples/ci_smoke/ci_smoke.ino`
  Minimal construction example.
- `test/test_pc/test_max2871.cpp`
  Native unit tests for math and interface behavior.
- `test/test_feather/test_feather.cpp`
  Hardware-oriented integration test.
- `platformio.ini`
  Build and test environments.
- `library.properties`
  Arduino Library Manager metadata.
- `library.json`
  PlatformIO library metadata.

## Architecture

The codebase uses three layers.

### 1. Device interface layer

`I_PLLSynthesizer` defines the API expected by application code:

- `begin()`
- `setFrequency(double freqMHz)`
- `setFrequency(uint32_t fmn, uint8_t diva)`
- `outputSelect(RFOutPort port)`
- `outputPower(int dBm, RFOutPort port = RF_ALL)`
- `isLocked()`

This layer exists so future drivers such as ADF4356 or LMX2594 can be swapped in without changing upper-level control code.

### 2. Transport and MCU layers

`I_MAX2871Transport` defines the chip-facing operations the driver needs:

- `spiWriteRegister`
- `readMuxout`

`IMCUHAL` defines the controller-facing primitives:

- `pinMode`
- `digitalWrite`
- `digitalRead`
- `delayMs`
- `spiBegin`
- `spiEnd`
- `spiTransfer16`

`IDelayProvider` narrows the timing dependency used by the driver itself:

- `delayMs`

The driver never calls Arduino APIs directly. It talks to an `I_MAX2871Transport` for chip-level operations and an `IDelayProvider` for timing.

`hal.h` provides the shared pin enums and the inline bitfield helpers used by the register writer:

- `bitMask(bit_hi, bit_lo)`
- `fieldValue(value, bit_hi, bit_lo)`

### 3. Concrete MAX2871 driver

`MAX2871` implements `I_PLLSynthesizer` and owns:

- the reference frequency in MHz
- a reference to an `I_MAX2871Transport`
- a reference to an `IDelayProvider`
- a shadow register image
- the computed synthesizer values `Frac`, `M`, `N`, `DIVA`, `Fpfd`, and `R`
- a dirty-register bitmask
- a one-time startup flag

## MAX2871 object model

The class stores register state in:

```cpp
struct max2871Registers {
    static constexpr uint8_t numRegisters = 7;
    uint32_t Reg[numRegisters];
};
```

Important implementation detail:

- the array has 7 slots
- the current code actively programs only registers `R5` through `R0`
- dirty-mask logic is initialized for 6 writable registers with `0x3F`

When recreating the project, preserve this behavior because the tests and startup path assume it.

## Default register image

The driver starts from a fixed default register set stored as `MAX2871::defaultRegisters`.

The active defaults represent this baseline:

- reference clock: 66.0 MHz
- target baseline comments: 60.0 MHz
- digital lock detect enabled in register 2
- RF output A off
- RF output B on at +5 dBm
- output divider set for divide-by-64

The default image acts as the reset state. Runtime calls then modify specific bitfields on top of it.

## Public behavior

### Construction

Constructor signature:

```cpp
explicit MAX2871(double refMHz, I_MAX2871Transport& transport, IDelayProvider& timing);
```

Rules:

- there is no default constructor
- the caller must provide the reference frequency
- the caller must provide a transport object and a timing provider

### Initialization

`begin()` just calls `reset()`.

`reset()`:

1. sets `first_init = true`
2. copies `defaultRegisters` into the mutable shadow registers `Curr`
3. calls `updateRegisters()`
4. sets `first_init = false`

### Frequency programming

There are two paths.

`setFrequency(double freqMHz)`:

- computes `Frac`, `M`, `N`, `DIVA` by calling `freq2FMN(freqMHz)`
- writes those values into register fields
- pushes changes with `updateRegisters()`

`setFrequency(uint32_t fmn, uint8_t diva)`:

- treats `fmn` as a packed value
- extracts:
  - `Frac` from bits `[31:20]`
  - `M` from bits `[19:8]`
  - `N` from bits `[7:0]`
- stores `DIVA = diva`
- writes those fields into the shadow registers
- pushes changes with `updateRegisters()`

### Output selection

`outputSelect(RFOutPort port)` controls the output enable bits in register 4:

- `R4[8]` controls RF output B
- `R4[5]` controls RF output A

Supported selections:

- `RFNONE`
- `RF_A`
- `RF_B`
- `RF_ALL`

### Output power

`outputPower(int dBm, RFOutPort port)` maps dBm values to the chip bitfields:

- `-4 dBm -> 0`
- `-1 dBm -> 1`
- `+2 dBm -> 2`
- `+5 dBm -> 3`

Register mapping:

- `R4[4:3]` sets output A power
- `R4[7:6]` sets output B power

Any unsupported dBm value is ignored and leaves the current setting unchanged.

### Lock detect

`isLocked()` returns the result of `I_MAX2871Transport::readMuxout()`.

The driver assumes MUXOUT has been configured to expose digital lock detect in the default register image.

The startup delay after register 5 is sourced from `IDelayProvider::delayMs(20)`.

## Frequency calculation algorithm

The project uses a simple fractional-N search implemented in `freq2FMN(float target_freq_MHz)`.

Recreate it exactly as follows:

1. Set `R = 1`.
2. Compute `Fpfd = refMHz / R`.
3. Start with `Fvco = target_freq_MHz`.
4. While `Fvco < 3000.0`, repeatedly double `Fvco` and increment `DIVA`.
5. Compute `NdotF = Fvco / Fpfd`.
6. Set `N` to the integer portion of `NdotF`.
7. Set `floatFrac = NdotF - N`.
8. Search `M_candidate` from `4095` down to `2`.
9. For each candidate:
- compute `F_candidate = floor(floatFrac * M_candidate)` by truncating to an integer
- compute `FvcoCalculated = Fpfd * (N + F_candidate / M_candidate)`
- compute absolute error against the target `Fvco`
- keep the best `(F_candidate, M_candidate)` pair
- stop early if the error reaches zero

10. Set:
- `Frac = round(best_F)`
- `M = best_M`

Reverse conversion in `fmn2freq()` is:

```cpp
fVCO = Fpfd * (N + (double)Frac / M);
fout = fVCO / (1 << DIVA);
```

### Frequency range behavior

The code is intended for the MAX2871 operating range of 23.5 MHz to 6000.0 MHz.

Observed behavior from tests:

- 23.5 MHz round-trips correctly
- 6000.0 MHz round-trips correctly
- integer-N cases produce `Frac = 0`
- representative values such as 100, 915, 1420, 2400, 3600, and 5800 MHz round-trip within a tight tolerance

## Register programming model

The driver keeps a mutable shadow image in `Curr.Reg[]` and only writes registers marked dirty.

### Low-level field update

`setRegisterField(regAddr, bit_hi, bit_lo, value)` performs:

1. bit-order normalization if `bit_lo > bit_hi`
2. validation
   - reject writes that touch address bits below bit 3
   - reject writes outside bit 31
   - reject register numbers above 6
3. mask generation via `bitMask`
4. field alignment via `fieldValue`
5. writeback only if the new register value differs
6. dirty-bit propagation rules

Dirty propagation rules are important:

- if register 1 changes, register 0 is also marked dirty
- if register 4 changes, register 0 is also marked dirty

This preserves the current driver behavior around double-buffered or dependent register updates.

### Programming sequence

`writeRegister(uint32_t value)` is only a thin wrapper over `I_MAX2871Transport::spiWriteRegister(value)`.

`updateRegisters()` has two modes.

First-startup mode, executed once after reset:

1. write register 5
2. delay 20 ms
3. write register 4 with both RF outputs forcibly disabled
4. write registers 3, 2, 1, 0 in descending order
5. mark registers 5 through 0 dirty again for the second cycle

Normal mode:

- walk from register 5 down to 0
- write only registers whose dirty bit is set
- clear the dirty mask after the pass

The startup behavior is deliberate. The source comments say the first cycle ensures a clean-clock startup and that the second cycle starts the VCO selection process.

## Board implementations

The concrete board-side code remains in the existing board files, but the driver no longer depends on the mixed `HAL` interface directly. The board objects implement both `I_MAX2871Transport` and `IMCUHAL`, and because `IMCUHAL` inherits `IDelayProvider`, the same object can satisfy both constructor parameters.

## Build and packaging design

The project is both:

- a standard Arduino library
- a PlatformIO development project

### Arduino packaging

`library.properties` exposes the library to the Arduino ecosystem.

### PlatformIO packaging

`library.json` exposes the library to PlatformIO.

### PlatformIO environments

`platformio.ini` defines these environments:

- `native`
  Runs PC-native Unity tests and builds source with `test_build_src = yes`.
- `feather`
  Runs hardware tests against an Adafruit Feather RP2040-style target.
- `uno`
  Builds the standalone Arduino target.
- `mega`
  Builds the standalone Arduino target for Mega 2560.

`src/main_entry.cpp` exists so non-library and test targets have a valid entrypoint when needed.

## Test strategy

The current design is validated in two ways.

### Native tests

`test/test_pc/test_max2871.cpp` verifies:

- known-value frequency round trips
- min/max frequency boundaries
- integer-N behavior
- parameterized round trips across common RF frequencies
- interface-level usage through `I_PLLSynthesizer`
- output-select register effects

These tests run without hardware by using `MockHAL`.

### Hardware tests

`test/test_feather/test_feather.cpp` verifies:

- initialization runs on target hardware
- MUXOUT can be observed
- frequency calculation round-trips on hardware
- output enable/power calls execute in a real deployment path

## Example usage

The minimal intended application flow is:

```cpp
#include <Arduino.h>
#include "max2871.h"
#include "arduino_hal.h"

static constexpr uint8_t RF_EN   = 5;
static constexpr uint8_t PIN_LE  = A3;
static constexpr uint8_t PIN_MUX = A2;
static constexpr double  REF_MHZ = 60.0;

ArduinoHAL hal(PIN_LE, RF_EN, PIN_MUX);
MAX2871 lo(REF_MHZ, hal, hal);

void setup() {
    hal.begin();
    lo.begin();
    hal.setCEPin(true);
    lo.setFrequency(42.0);
}

void loop() {}
```

Expected caller responsibilities:

- instantiate a concrete board object that satisfies both interfaces
- pass the correct reference frequency
- initialize the underlying board support as needed before driver use
- call `lo.begin()` before setting frequency
- enable CE if the target board requires it

## How to recreate this project

1. Create an Arduino library repository with `src/`, `examples/`, `test/`, `library.properties`, `library.json`, and `platformio.ini`.
2. Define `I_PLLSynthesizer` with the exact API listed above.
3. Define `HAL` with the exact GPIO, timing, SPI, CE, and MUXOUT methods listed above.
4. Add inline `bitMask` and `fieldValue` utilities to `hal.h`.
5. Implement `MAX2871` as a concrete `I_PLLSynthesizer` using:
   - a reference-frequency constructor
   - a shadow register image
   - default register constants
   - cached synthesizer fields
   - a dirty-mask update model
6. Implement the frequency conversion algorithm exactly as documented in this file.
7. Implement `setRegisterField()` and preserve the dirty propagation from register 1 to 0 and register 4 to 0.
8. Implement `updateRegisters()` with the two-phase startup write sequence and the 20 ms delay after register 5.
9. Implement `ArduinoHAL` using Arduino `SPI`, SPI mode 0, MSB-first writes, and LE pulse latching.
10. Implement `MockHAL` and `SmokeHAL` for tests and compile-only builds.
11. Add native Unity tests for round-trip math, boundary values, interface usage, and register-side effects.
12. Add one minimal example sketch that constructs the HAL and driver.
13. Add a stub `main_entry.cpp` so PlatformIO standalone and test targets link cleanly.

## Invariants to preserve

- public frequency API uses MHz
- the driver depends only on `HAL`, never on direct Arduino calls
- the first `begin/reset` path uses the special startup programming sequence
- runtime writes operate on a shadow register image, not ad hoc register literals
- output enable and power are handled exclusively through register 4 bitfields
- lock state comes from MUXOUT through the HAL
- the library must build on Arduino and run native tests on a host machine

## Validation status

This design was reconstructed from the current codebase and checked against the existing native test suite.

Native verification result:

- `pio test -e native` passed with 7/7 test cases succeeding
