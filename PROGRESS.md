# Progress

## Current Direction

The next focus is testing.

## Plan

- [ ] Review the current tests against the stand-alone MAX2871 library goal.
- [ ] Remove or rewrite tests that still reflect the old mixed-project HAL design.
- [ ] Get the remaining test/build baseline into a trustworthy green state.
- [ ] Combine the common HAL files into one shared hardware HAL.
- [ ] Clean up and remove unused source code and files after the HAL merge and test rewrite are settled.

## Completed Cleanup So Far

- Removed the unused `spiBegin()` HAL hook.
- Removed dead helper methods from `MockHAL` and `SmokeHAL`.
- Removed ADC-related HAL API and implementation spillover.
- Normalized several HAL style inconsistencies across `arduino_hal.h` and related files.
- Fixed the `ci_smoke_wrapper.cpp` preprocessor warning.
- Created the `pre-hal-unification` tag as a checkpoint before HAL unification work.
