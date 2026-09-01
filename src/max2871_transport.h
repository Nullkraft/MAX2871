/* max2871_transport.h
   MAX2871-specific transport contract.

   This interface sits between the pure MCU HAL and the MAX2871 driver.
   The caller owns SPI transactions and LE timing around register transfers.
 */

#ifndef MAX2871_TRANSPORT_H
#define MAX2871_TRANSPORT_H

#include <stdint.h>

class I_MAX2871Transport {
public:
    virtual ~I_MAX2871Transport() {}

    virtual void spiWriteRegister(uint32_t value) = 0;
    virtual bool readMuxout() = 0;
};

#endif // MAX2871_TRANSPORT_H
