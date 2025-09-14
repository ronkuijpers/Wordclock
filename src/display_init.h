#pragma once

#include "led_state.h"
#include "display_settings.h"
#include "log.h"

// Initialiseer LED en display instellingen
inline void initDisplay() {
    ledState.begin();
    displaySettings.begin();
    logInfo("🟢 LED en display instellingen geinitialiseerd");
}
