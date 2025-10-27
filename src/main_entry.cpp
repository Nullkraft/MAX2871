#if defined(PIO_UNIT_TESTING)

#elif defined(ARDUINO)
#if defined(MAX2871_STANDALONE)
#include <Arduino.h>

void setup()
{
}

void loop()
{
}
#endif
#else
int main()
{
    return 0;
}
#endif
