#include <etl/error_handler.h>

// Platform specific
#include "Platform.h"

// Common
#include "ADC.h"
#include "DAC.h"
#include "Compressor.h"
#include "CPULoad.h"

int main() {
    CPULoad cpuLoad; // init singleton

    etl::set_assert_function(Platform::error_handler);

    Platform platform;

    ADC adc1;
    Compressor compressor1(adc1);
    DAC dac1(compressor1);

    while (true) {
        dac1.process();
    }
    return 0;
}
