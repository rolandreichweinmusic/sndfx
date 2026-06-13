#pragma once

#include "Operation.h"
#include "AudioDevice.h"

class ADC : public Operation
{
public:
    ADC();
    void process() override;

    static AudioDevice& device() { return _device; }

private:
    static AudioDevice _device; // singleton for shared access to audio device by ADC and DAC
};