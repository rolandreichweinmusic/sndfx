#include "DAC.h"

DAC::DAC(Operation& input) : input(input) {}

void DAC::process() {
    input.process();
    // Add DAC logic here
}
