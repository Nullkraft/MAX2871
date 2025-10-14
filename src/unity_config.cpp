/* During 'pio test', PlatformIO's test runner provides unity_config.cpp
 *
 * During 'pio run', you provide unit_config.cpp but it must be hidden
 * when you are running 'pio test' so theirs can take over.
 */

/* *** If PIO_UNIT_TESTING IS defined ***
 * Use serial configuration provided by platformio.ini...
 *
 * 'pio test' build
 *   '--> PlatformIO defines PIO_UNIT_TESTING
 *        '--> Your unity_config.cpp is excluded
 *             '--> PlatformIO's implementation handles Unity output
 *                  '--> Your unit_config.h still provides per-project macros
*/
#ifndef PIO_UNIT_TESTING

/* *** If PIO_UNIT_TESTING is NOT defined ***
 * Compile this unity_config.cpp so you can use actual hardware serial...
 *
 * 'pio run -e uno' or 'pio run -e uno_example' builds
 *   '--> PIO_UNIT_TESTING is not defined
 *        '--> Your unity_config.cpp is compiled
 *             '--> Your implementation handles Unity output
 *                  '--> Your unit_config.h still provides per-project macros
 */
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
