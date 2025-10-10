#ifdef CI_SMOKE_BUILD
// Wrap the Arduino example sketch so PlatformIO can compile it as part of the
// standard project sources.
#include "../examples/ci_smoke/ci_smoke.ino"
#endif
