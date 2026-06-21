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
    Operation& input;
    PIO _pio;
    uint _sm;
    int _dma_chan;
    uint _offset;

    // Stereo I2S staging buffer. The DSP pipeline is mono (the PCM1808 ADC
    // captures only the left slot), but the PCM5102A is clocked for stereo I2S
    // and consumes two 32-bit words per frame. Each mono sample is duplicated
    // into the left and right slots here before being streamed to the PIO, so
    // both DAC outputs carry the same signal and exactly one input sample is
    // consumed per output frame (keeping the ADC and DAC sample rates matched).
    etl::array<SampleType, bufferSize * 2> _stereo;
};
