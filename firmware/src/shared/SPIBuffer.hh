#ifndef _SPIBUFFER_H
#define _SPIBUFFER_H
#include "AtomicCircularBuffer.hh"

#define SPIDATA_BUFFER_SIZE 128

class SPIBuffer
{

    static AtomicCircularBuffer<uint8_t, SPIDATA_BUFFER_SIZE> tx_buf;
    static volatile bool tx_isTransmitting;

public:

    inline void write(uint8_t b)
    {
        while(!tx_buf.push(b)){} // wait in a loop if there is no room in the buffer
        // we are assuming that the ISR can't send the above message before we exit
        // otherwise we need to disable the interrupt before to prevent a race on tx_isTransmitting
        if ( !tx_isTransmitting )
        {
            tx_isTransmitting = true;
            tx_isr_handler();
        }
    }

    void init(void);

    static void tx_isr_handler(void);
};
#endif
