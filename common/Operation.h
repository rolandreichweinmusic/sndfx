#pragma once

#include <cstddef>

#include <etl/array.h>

class Operation
{
public:
    virtual ~Operation() = default;
    virtual void process() = 0;

    using SampleType = uint32_t;
    static constexpr size_t bufferSize = 256;
    using BufferType = etl::array<SampleType, bufferSize>;

    BufferType& getBuffer();

protected:
    BufferType _buffer;
};
