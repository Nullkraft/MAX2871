#include <Arduino.h>

#if !defined(UNIT_TEST) && !defined(PIO_UNIT_TESTING) && !defined(CI_SMOKE_BUILD)
// Wrap the Arduino example sketch so PlatformIO can compile it as part of the
// standard project sources.
#include "../examples/specAnn/specAnn.ino"
#endif
