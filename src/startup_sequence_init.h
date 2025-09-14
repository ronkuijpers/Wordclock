#pragma once

#include "sequence_controller.h"
#include "log.h"

// Initialiseer en start de startup animatie
inline void initStartupSequence(StartupSequence& startupSequence) {
    startupSequence.start();
    logInfo("🟢 StartupSequence gestart");
}

// Update de startup animatie
inline bool updateStartupSequence(StartupSequence& startupSequence) {
    if (startupSequence.isRunning()) {
        startupSequence.update();
        return true; // animatie actief, klok nog niet tonen
    }
    return false; // animatie klaar
}
