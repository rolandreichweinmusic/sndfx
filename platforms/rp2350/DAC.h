#pragma once

#include "Operation.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include <etl/array.h>

class DAC : public Operation
{
public:
    DAC(Operation& input, PIO pio = pio1, uint sm = 0, uint data_pin = 26, uint bclk_pin = 27);
    void process() override;

private:
    // DMA completion handler (DMA_IRQ_1). Re-arms playback with the next buffer
    // so BCLK/LRCLK never stall, and releases the just-played buffer for refill.
    static void dma_isr();
    void onDmaComplete();

    Operation& input;
    PIO _pio;
    uint _sm;
    uint _offset;
    int _dma_chan;

    // Two stereo I2S staging buffers for continuous (gapless) playback. The DSP
    // pipeline is mono (the PCM1808 ADC captures only the left slot), but the
    // PCM5102A is clocked for stereo I2S (two 32-bit words per frame). Each mono
    // sample is duplicated into the left and right slots, so both DAC outputs
    // carry the same signal and exactly one input sample is consumed per frame.
    //
    // The DMA ping-pongs between the two buffers, re-armed from the completion
    // ISR with no CPU gap, so the PCM5102A always sees a continuous bit clock
    // and keeps its internal PLL locked. A stalled BCLK drops that lock and the
    // codec mutes, which is why a one-shot, blocking DMA produced only silence.
    using StereoBuffer = etl::array<SampleType, bufferSize * 2>;
    StereoBuffer _stereo[2];

    volatile unsigned _playIndex; // buffer the DMA is currently streaming (ISR-owned)
    volatile unsigned _freeIndex; // buffer released by the ISR, ready to refill
    volatile bool _freeReady;     // set by the ISR when _freeIndex is valid

    static DAC* _instance;        // single instance, reached from the static ISR
};
