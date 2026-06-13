#pragma once

#include "Operation.h"

class Compressor : public Operation
{
public:
    Compressor(Operation& input);
    void process() override;
private:
    Operation& input;
};