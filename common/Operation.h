#pragma once

class Operation
{
public:
    virtual ~Operation() = default;
    virtual void process() = 0;

};
