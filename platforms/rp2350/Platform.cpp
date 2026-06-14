#include "Platform.h"

Platform::Platform() {
    // Nothing to do. With no operating system or scheduler, the audio loop runs
    // uninterrupted on the core at full priority, so real-time behaviour is
    // inherent. Any future board-level initialisation belongs here.
}
