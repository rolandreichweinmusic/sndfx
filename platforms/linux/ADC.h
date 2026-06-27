#pragma once

#include "Operation.h"
#include "AudioDevice.h"

class ADC : public Operation
{
public:
    ADC();
    void process() override;

    // Shared audio device for ADC (capture) and DAC (playback). Constructed on
    // first use (from main(), via DAC's constructor) rather than at static-init
    // time. AudioDevice's constructor opens the ALSA device and fires an
    // ETL_ASSERT on failure, so it must run after main() has installed the
    // handler via etl::set_assert_function(). A non-local static would be
    // constructed before main() and abort through ETL's default_assert instead.
    static AudioDevice& device() {
        static AudioDevice instance;
        return instance;
    }
};
