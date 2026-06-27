#include "Platform.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <print>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

namespace {
// Real-time priority for the audio thread. Kept below the top of the SCHED_FIFO
// range (1..99) so kernel threads (watchdog, IRQ handlers) can still preempt us.
// The audio loop blocks on ALSA every period, so it yields the core regularly
// and will not starve the rest of the system.
constexpr int audioRtPriority = 80;
} // namespace

Platform::Platform() {
    // Lock all current and future pages into RAM. A major page fault inside the
    // audio loop would stall it for milliseconds and cause an xrun.
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::print(stderr,
                   "Platform: mlockall failed ({}); audio may glitch under "
                   "memory pressure. Raise the RLIMIT_MEMLOCK limit to fix.\n",
                   strerror(errno));
    }

    // Switch the calling (audio) thread to the SCHED_FIFO real-time policy so it
    // preempts ordinary tasks and meets the ALSA period deadlines.
    // pthread_setschedparam returns the error number directly and does not set
    // errno, so it is passed straight to strerror.
    sched_param param{};
    param.sched_priority = audioRtPriority;
    int err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (err != 0) {
        std::print(stderr,
                   "Platform: could not enable SCHED_FIFO real-time scheduling "
                   "({}); running at normal priority. Grant CAP_SYS_NICE or "
                   "raise the RLIMIT_RTPRIO limit to fix.\n",
                   strerror(err));
    }
}

void Platform::error_handler(const etl::exception& e) {
    std::cerr << "ETL error: " << e.what() << std::endl;
    exit(1);
}

extern "C" void etl_putchar(int c)
{
    // Expand newlines to CR+LF so lines break correctly on a terminal.
    if (c == '\n') {
        std::putchar('\r');
    }
    std::putchar(c);
}
