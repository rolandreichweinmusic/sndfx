#include "Compressor.h"

Compressor::Compressor(Operation& input) : input(input) {}

void Compressor::process() {
    input.process();
    // Add compression logic here
}
