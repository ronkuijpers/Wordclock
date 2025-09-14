#pragma once

#include "sequence_controller.h"
#include "log.h"

// Initialize and start the startup animation
// This function starts the clock's startup animation.
// Called after setup so the clock only becomes active after the animation.
inline void initStartupSequence(StartupSequence& startupSequence) {
    startupSequence.start();
    logInfo("🟢 StartupSequence started");
}

// Update the startup animation
// This function updates the animation and indicates whether the clock can be shown yet.
inline bool updateStartupSequence(StartupSequence& startupSequence) {
    if (startupSequence.isRunning()) {
        startupSequence.update();
        return true; // animatie actief, klok nog niet tonen
    }
    return false; // animatie klaar
}
