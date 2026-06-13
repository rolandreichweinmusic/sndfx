#pragma once

#include "Operation.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

class ADC : public Operation
{
public:
    ADC(PIO pio = pio0, uint sm = 0, uint data_pin = 16, uint bclk_pin = 17, uint lrclk_pin = 18);
    void process() override;

private:
    PIO _pio;
    uint _sm;
    int _dma_chan;
    uint _offset;
};
