#if defined(PIO_UNIT_TESTING)
// Unit tests supply their own entry points; nothing to do here.

#elif defined(ARDUINO) && defined(MAX2871_STANDALONE)
#include <Arduino.h>

// Minimal sketch required so the Arduino core links successfully.
void setup()
{
}

void loop()
{
}
#else
// Native builds look for a standard C++ entry point.
int main()
{
    return 0;
}
#endif
