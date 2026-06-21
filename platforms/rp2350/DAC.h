#pragma once

#include "Operation.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

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
};
