#pragma once

#include "Operation.h"

class ADC : public Operation
{
public:
    ADC();
    void process() override;
};