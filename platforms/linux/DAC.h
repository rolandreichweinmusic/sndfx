#pragma once

#include "Operation.h"
#include "AudioDevice.h"

class AudioDevice;

class DAC : public Operation
{
public:
    DAC(Operation& input);
    void process() override;

private:
    AudioDevice& _device;
    Operation& input;
};
