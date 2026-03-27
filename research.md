# MAX2871 Library – Deep Dive Notes

## Project Purpose and Scope
- Arduino-focused library that drives the MAX2871 wideband PLL/VCO (23.5 MHz – 6 GHz) with an extensible hardware abstraction layer (HAL) so multiple boards and communication back-ends share the same synthesizer logic (`README.md`, `library.properties`).
- PlatformIO project layout with dual life as an Arduino library: `src/` holds implementation, `examples/` contain sketches, `test/` hosts Unity suites for PC-native and on-device validation, and `platformio.ini` defines build/test environments.
- Positioned as a building block for a broader spectrum-analyzer stack, with future placeholders for additional PLL chips (`I_PLLSynthesizer` mentions ADF4356, LMX2594, etc.).

## Code Architecture

### Core Driver (`src/max2871.cpp/h`)
- `MAX2871` implements `I_PLLSynthesizer`, exposing `setFrequency`, `outputSelect`, `outputPower`, and `isLocked`.
- Register shadowing: `max2871Registers { uint32_t Reg[7]; }` stores both defaults (`defaultRegisters`) and mutable state (`Curr`). Working copy mirrors the device and is the sole source for writes.
- Constructor is `explicit MAX2871(double refMHz, HAL& hal)`; the HAL reference is mandatory and immutable, preventing accidental unbound drivers.
- `begin()` simply calls `reset()`, which copies defaults into `Curr` and invokes `updateRegisters()` to perform Maxim’s two-pass “clean-clock startup”. `first_init` flag ensures the initial programming writes registers 5→0 twice (first with RF outputs disabled, then dirty tracked).
- `setFrequency(double)` delegates to `freq2FMN()`, then writes `M`, `Frac`, `N`, and `DIVA` bitfields in registers 1, 0, and 4 through `setRegisterField()`. The overload taking `(uint32_t fmn, uint8_t diva)` bypasses the math for precomputed tuples.
- `freq2FMN()` workflow:
  - Uses reference clock `_refMHz` and fixed `R = 1` to compute phase-detector frequency `Fpfd`.
  - Elevates the requested LO (`target_freq_MHz`) into the VCO’s legal 3–6 GHz band by doubling until in range, counting doublings into `DIVA`.
  - Computes integer `N = floor(Fvco / Fpfd)` and fractional remainder.
  - Brute-force scans `M` from 4095 down to 2, tracking the `F` numerator that minimizes `|Fvco - (Fpfd * (N + F/M))|`. `Frac` is the rounded best `F`.
  - Leaves TODO hooks for optimization (mirrors notes in `Issues and Ideas.md` about reducing `M` churn and exploring continued fractions).
- Register write policy via `_dirtyMask` (6-bit). `setRegisterField()`:
  - Normalizes bit ranges, rejects attempts to touch address bits [2:0] or out-of-range registers.
  - Applies `bitMask`/`fieldValue` helpers from `hal.h`, compares old/new value, and marks the register dirty.
  - Adds side effects: if Reg1 (fractional numerator) or Reg4 (output control) change, it ORs in downstream bits so Reg0 is automatically reprogrammed (required for MAX2871’s double-buffered design).
- `updateRegisters()` honors `first_init` (initial programming writes 5→0 with RF outputs temporarily disabled, `delayMs(20)` between writes) then, on subsequent calls, iterates dirty bits 5→0 and writes only changed registers through the HAL.
- Output control helpers:
  - `outputSelect()` toggles RF A/B enable bits in R4 (bits 5,8) based on `RFOutPort` flags.
  - `outputPower()` maps dBm to 2-bit codes and updates R4[7:3]. Invalid input silently ignores.
- Status: `isLocked()` proxies to `HAL::readMuxout()`. Actual behavior depends entirely on HAL implementation; several HALs stub this.
- Public telemetry: `Frac`, `M`, `N`, `DIVA`, `Fpfd`, and `Curr` registers are exposed, enabling external diagnostics (examples print them). There are leftover debug members `print_val1/2` flagged “JUNK Delete Me”.

### HAL Interface and Implementations (`src/hal.h`, derived headers)
- `HAL` defines virtual hooks for SPI, GPIO, timing, MAX2871 register writes, chip-enable control, lock detect, and ADS7826 ADC reads.
- Helper inline functions `bitMask` and `fieldValue` (32-bit) are generalized bitfield utilities used throughout the driver.
- Implementations:
  - `BitBangHAL` (GPIO bit-banging; examples/EvalBoardBasic & `test_eval_board`): manages CLK/DATA/LE pins via Arduino core `shiftOut`, optional CE line, but stubs `readMuxout()` (always `true`) and `readADC()`.
  - `ArduinoHAL` (classic AVR SPI) and `FeatherHAL` (RP2040 SPI): identical patterns—`SPI.beginTransaction` with latch pulsing; optional CE and MUX pins, supports dual ADS7826 chip selects. Both expose `setSpiClockHz()` (default 8 MHz). `readMuxout()` returns digital read of the configured pin. ADC read shifts data for a 10-bit result left-justified in 12 bits.
  - `MockHAL` (PC tests): logs up to seven register writes for assertions, returns `false` for lock detect, implements no-delay/digital side effects.
  - `SmokeHAL` (CI smoke build): pure no-op HAL fulfilling interface for minimal compilation.
- HALs typically offer an ad-hoc `begin()` method (not in `HAL` interface) to initialize board pins; sketches/tests must remember to call it.

### Frequency Calculator Module (`src/frequency_calculator.*`)
- Coordinates three synthesizers (LO1/LO2/LO3) implementing `I_PLLSynthesizer`. Tracks IF stages (`IF1`, `IF2`, `IF3`), reference clocks, selectable high/low-side injection for each LO, and exposes computed LO outputs.
- `set_LO_frequencies(rfin, RefClock, R)`:
  - Computes IF1 step aligned to the reference’s phase detector frequency.
  - Chooses LO1 high- or low-side mixing based on a threshold tied to the reference clock (`threshold = 2343.0001 MHz` when using `RefClock1`, else `2403.2731`).
  - Programs LO1/LO2/LO3 via `setFrequency()` while caching FFT-friendly IF values.
- Designed for the spectrum-analyzer example; future TODOs (calibration/spur mitigation) are hinted in comments.

## Build and Integration Surface
- `platformio.ini` defines multiple environments:
  - `native`: PC tests (`test_pc`) with `MockHAL`.
  - `uno`/`mega`: AVR build targets with `MAX2871_STANDALONE`.
  - `smoke`: Lightweight Arduino build bundling `examples/ci_smoke`.
  - `feather`: RP2040 standalone build target (`MAX2871_STANDALONE`).
- `src/main_entry.cpp` neutralizes entry points depending on `PIO_UNIT_TESTING`, `ARDUINO`, and `MAX2871_STANDALONE` macros so PlatformIO/Arduino coexist.
- `ci_smoke_wrapper.cpp` inlines the smoke sketch into a compilation unit for PlatformIO builds without needing Arduino’s sketch preprocessor.
- `unity_config.cpp/h` provide conditional serial output shims: when PlatformIO runs tests (`PIO_UNIT_TESTING` defined) its own transport takes over; otherwise, Arduino `Serial` or `stdout` is wired up.
- `library.properties` advertises the library as `MAX2871_EvalBoard` version `0.1.0`, architecture `*`, with a short feature summary.
- `Makefile` adds a non-PlatformIO workflow: downloads `arduino-cli` locally, provides `make ci` target that compiles the smoke sketch with Arduino CLI and runs native tests via `pio test`.

## Examples and Usage Patterns (`examples/`)
- `EvalBoardBasic/EvalBoardBasic.ino`: Bit-banged SPI on an Arduino with 60 MHz reference. Demonstrates initialization, enabling RF outputs, power selection, and a simple frequency sweep. Highlights requirement to call `hal.begin()`, `lo.begin()`, `hal.setCEPin(true)`.
- `ci_smoke/ci_smoke.ino`: Minimal smoke test to verify compilation linkage; instantiated via `ci_smoke_wrapper.cpp`.

## Test Coverage (`test/`)
- `test_pc/test_max2871.cpp` (PlatformIO native target):
  - Exercises `freq2FMN` across boundary and representative frequencies (23.5 MHz minimum to 6 GHz maximum) with `MockHAL`.
  - Ensures integer-N case zeros `Frac`.
  - Validates `outputSelect()` toggles register bits correctly.
  - Verifies interface-based usage through `I_PLLSynthesizer*`.
- `test_hw` and `test_feather` (Arduino targets):
  - Shared structure verifying `begin()` on hardware, observing MUXOUT (informational), checking frequency round-trip (`setFrequency` vs `fmn2freq`).
  - Configure board-specific pins and ensure reference enables are asserted before tuning.
- `test_eval_board/test_eval_max2871.cpp`: Tailored to the DIY eval board with `BitBangHAL`. Adds register dump helpers and a power-level verification suite using `TEST_ASSERT_BITS`.
- All tests rely on Unity, with Arduino-friendly timeouts and optional serial messaging (`TEST_MESSAGE`).

## Supporting Assets and Data
- `docs/`:
  - Datasheet PDFs (`MAX2871.pdf`, register map).
  - Design note “The Include-Chain vs. Inheritance.txt” explaining header dependency direction (HAL vs synthesizer interfaces).
  - Alternative algorithm sketch “deepseek optimized freq2FMN function.cpp” exploring integer arithmetic for FMN calculation (not integrated).
- `Issues and Ideas.md`: Backlog of optimization ideas (reduce `M` churn, capture multiple minimal-error `M` values) and general tooling TODOs; future entries will track the transition to precomputed FMN tables.
- `LO2_ref1_hi_fmn_list.csv`: Pairings of FMN integers with precise LO2 frequencies—likely empirical calibration data for analysis.
- `simulavr.info`: Full manual for SimulAVR, suggesting intent to simulate AVR behavior offline.
- `requirements.txt`: Reserved for Python 3.11.0 tooling needs during development automation or analysis.

## Known Limitations, TODOs, and Observations
- `BitBangHAL::readMuxout()` is hardcoded to `true`; lock-detect dependent logic (e.g., `isLocked()`) is meaningless when using this HAL until implemented.
- Example sketches manually call non-virtual `begin()` on HALs; there is no compile-time guard enforcing this, so forgetting to call it leaves pins unconfigured.
- `freq2FMN()` currently uses floating point and O(4095) loop per tune; this will remain the interim path until precomputed FMN tables (derived from artifacts like `LO2_ref1_hi_fmn_list.csv`) are generated after board bring-up.
- Default register set chooses RF_B enabled, RF_A disabled at +5 dBm with divide-by-64. Users wanting different defaults must adjust `defaultRegisters`.
- Debug members (`print_val1`, `print_val2`) are stale; header comment explicitly marks them for removal.
- Threshold constants in `FrequencyCalculator` are magic numbers tied to specific reference clocks; documentation suggests these will change post-calibration.
- `FrequencyCalculator` only supports R-divider values that fit in `uint8_t`; there’s no guard against invalid `R_in`.
- Some tests (`test_feather` and the old `test_hw` path) duplicated large blocks; future refactor might share fixtures but current instructions discourage multi-file edits.
- Lock-detect behavior via `readMuxout()` is intentionally deferred; HAL stubs may return fixed values until the RF board bring-up defines the expected semantics.

## Decisions and Forward Work
- **HAL initialization:** The base `HAL` contract will be extended with a virtual `begin()` so every implementation exposes a consistent setup entry point.
- **FMN strategy:** Brute-force FMN solving remains in place temporarily; once the RF board and individual PLLs are characterized, precomputed tables (e.g., derived from `LO2_ref1_hi_fmn_list.csv`) will replace the runtime search. Continued-fraction optimizations are no longer planned.
- **SpecAnn system role:** The SpecAnn hardware concept mounts three MAX2871 devices; coordinated tuning of LO1/LO2/LO3 forms the foundation of the target spectrum analyzer.
- **SpecAnn workflow:** Operational work for the saTech importer will follow after the individual PLL validation work wraps.
- **Lock detect:** `readMuxout()` implementations can remain stubbed until post bring-up measurements dictate actual behavior.
