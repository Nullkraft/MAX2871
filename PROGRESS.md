# Progress

## Current Direction

The next focus is pruning or retaining the remaining example/HAL paths deliberately.

## Plan

- [x] Review the current tests against the stand-alone MAX2871 library goal.
- [x] Remove or rewrite tests that still reflect the old mixed-project HAL design.
- [x] Get the remaining test/build baseline into a trustworthy green state.
- [x] Combine the common HAL files into one shared hardware HAL.
- [x] Clean up and remove unused source code and files after the HAL merge and test rewrite are settled.
- [x] Keep the `ci_smoke` path (`examples/ci_smoke`, `smoke_hal.h`, `ci_smoke_wrapper.cpp`, and the Arduino CLI smoke build in `Makefile`) since GitHub Actions still depends on it and is currently green.
- [x] Remove the dedicated evaluation-board example/test path and leave the related commented `env:eval_board` wiring for now.
- [x] Keep `mock_hal.h` unless the native test strategy changes, since `test/test_pc/test_max2871.cpp` still depends on it.

## Completed Cleanup So Far

- Removed the unused `spiBegin()` HAL hook.
- Removed dead helper methods from `MockHAL` and `SmokeHAL`.
- Removed ADC-related HAL API and implementation spillover.
- Normalized several HAL style inconsistencies across `arduino_hal.h` and related files.
- Fixed the `ci_smoke_wrapper.cpp` preprocessor warning.
- Created the `pre-hal-unification` tag as a checkpoint before HAL unification work.
- Removed the old `test_hw` path and the mixed `specAnn` / `tune_lo` example paths.
- Removed the redundant `feather_hal.h` implementation and repointed the Feather test to `ArduinoHAL`.
