#pragma once

#include "Operation.h"

class DAC : public Operation
{
public:
    DAC(Operation& input);
    void process() override;
private:
    Operation& input;
};