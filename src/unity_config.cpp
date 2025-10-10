#ifndef PIO_UNIT_TESTING

#include "unity_config.h"

#ifdef ARDUINO
#    include <Arduino.h>
#else
#    include <cstdio>
#endif

extern "C" {

void unityOutputStart(unsigned long baudrate)
{
#ifdef ARDUINO
    Serial.begin(baudrate);
#else
    (void)baudrate;
#endif
}

void unityOutputChar(unsigned int c)
{
#ifdef ARDUINO
    Serial.write(static_cast<uint8_t>(c));
#else
    putchar(static_cast<int>(c));
#endif
}

void unityOutputFlush(void)
{
#ifdef ARDUINO
    Serial.flush();
#else
    fflush(stdout);
#endif
}

void unityOutputComplete(void)
{
#ifdef ARDUINO
    Serial.end();
#endif
}

} // extern "C"

#endif // !PIO_UNIT_TESTING
