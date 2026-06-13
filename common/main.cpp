#include "etl/vector.h"

#include "ADC.h"
#include "DAC.h"
#include "Compressor.h"

int main() {
    ADC adc1;
    Compressor compressor1(adc1);
    DAC dac1(compressor1);

    while (true) {
        dac1.process();
    }
    return 0;
}
