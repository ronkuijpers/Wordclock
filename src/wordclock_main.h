#pragma once

#include "wordclock.h"
#include "log.h"

// Hoofdlogica van de klok
inline void runWordclockLoop() {
    wordclock_loop();
}
