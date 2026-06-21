#pragma once

#include "Operation.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

class ADC : public Operation
{
public:
    ADC(PIO pio = pio0, uint sm = 0, uint data_pin = 2, uint bclk_pin = 3, uint lrclk_pin = 4,
        uint sck_pin = 5, uint sck_sm = 1);
    void process() override;

private:
    // DMA completion handler (DMA_IRQ_0). Re-arms capture into the next buffer
    // so the RX FIFO is drained continuously, and publishes the filled buffer.
    static void dma_isr();
    void onDmaComplete();

    PIO _pio;
    uint _sm;
    uint _offset;
    int _dma_chan;

    // Two capture buffers for continuous double-buffered input. The DMA fills
    // one while process() consumes the other, so the PCM1808's RX stream is
    // drained without gaps and stays sample-aligned with playback. The mono
    // pipeline buffer returned by getBuffer() (Operation::_buffer) is filled
    // from whichever capture buffer the ISR most recently completed.
    BufferType _capture[2];

    volatile unsigned _captureIndex; // buffer the DMA is currently filling (ISR-owned)
    volatile unsigned _readyIndex;   // most recently completed buffer
    volatile bool _readyValid;       // set by the ISR when _readyIndex is valid

    static ADC* _instance;           // single instance, reached from the static ISR
};
