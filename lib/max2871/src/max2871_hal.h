#ifndef _MAX2871_HAL_
#define _MAX2871_HAL_

#include <stdint.h>

// Forward declare the MAX2871 driver class
// For future use
class MAX2871;

class Max2871Hal {
public:
    virtual void spiWriteRegister(uint32_t value) = 0;
    virtual void toggleLEPin() = 0;
    virtual void setCEPin(bool enabled) = 0;
    virtual bool readMuxout() = 0;

    virtual ~Max2871Hal() {}
};

#endif
