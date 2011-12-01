#include "Configuration.hh"
#if defined(SPI_SS_PIN)
#include "SPI.hh"
#include "SPIBuffer.hh"

AtomicCircularBuffer<uint8_t, SPIDATA_BUFFER_SIZE> SPIBuffer::tx_buf;
volatile bool SPIBuffer::tx_isTransmitting = false;

void SPIBuffer::init(void)
{
    SPI_SS_PIN::setDirection(true);
    SPI_SS_PIN::setValue(true);


    SPI.begin();
    SPI.setBitOrder(SPI_MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.attachInterrupt();
}

void SPIBuffer::tx_isr_handler(void) 
{
    SPI_SS_PIN::setValue(true);

    if ( !tx_buf.empty() )
    {
        uint8_t spi = tx_buf.front();
        tx_buf.pop();
        SPI_SS_PIN::setValue(false);
        SPI.transferNonBlocking(spi);
        // tx_isTransmissting was set by the initial write
    }
    else
    {
        tx_isTransmitting = false;
    }
}

ISR(SPI_STC_vect)
{
    SPIBuffer::tx_isr_handler();
}
#endif

