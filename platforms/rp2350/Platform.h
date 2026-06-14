#pragma once

// On the RP2350 the firmware owns the core outright: main() runs as the single
// bare-metal execution context with no operating system or scheduler competing
// for the CPU, so the audio loop is already effectively real-time. This type
// mirrors the Linux Platform so common code (main.cpp) has one uniform
// construction point for board-level initialisation across both targets.
class Platform {
public:
    Platform();
};
