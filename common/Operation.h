#pragma once

#include <cstddef>

#include <etl/array.h>

class Operation
{
public:
    virtual ~Operation() = default;
    virtual void process() = 0;

    // Signed: both ALSA (SND_PCM_FORMAT_S32_LE) and I2S carry two's-complement
    // PCM, and DSP math (level detection, gain, arithmetic shifts) needs signed
    // semantics. Saturate via a wider intermediate type rather than relying on
    // unsigned wrap.
    using SampleType = int32_t;
    static constexpr size_t bufferSize = 256;
    using BufferType = etl::array<SampleType, bufferSize>;

    BufferType& getBuffer();

protected:
    BufferType _buffer;
};
