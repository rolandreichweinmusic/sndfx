#pragma once

// Activates low-latency audio capabilities for the Linux/ALSA build:
//   * locks the process memory so page faults cannot stall the audio loop, and
//   * switches the audio thread to the SCHED_FIFO real-time scheduling policy
//     so it meets ALSA period deadlines.
//
// Activation is best-effort: it requires elevated privileges (CAP_SYS_NICE and
// suitable RLIMIT_MEMLOCK / RLIMIT_RTPRIO limits). If the process lacks them the
// failures are reported on stderr and execution continues at normal priority,
// trading latency headroom for the ability to run unprivileged.
class Platform {
public:
    Platform();
};
