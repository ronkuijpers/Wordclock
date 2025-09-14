#pragma once

#include "wordclock.h"
#include "log.h"

// Main logic of the clock
// This function executes the main logic of the clock (showing time, animations, etc).
// Called periodically from the main loop.
inline void runWordclockLoop() {
    wordclock_loop();
}
