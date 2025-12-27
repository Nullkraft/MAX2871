// Wrap the Arduino example sketch, specAnn.ino, so PlatformIO can compile
// it as part of a standard project

#ifdef PLATFORMIO
#include "../examples/specAnn/specAnn.ino"
#endif
