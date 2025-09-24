#pragma once

#include "led_state.h"
#include "display_settings.h"
#include "log.h"

// Initialize LED and display settings
// This function initializes the LED state and display settings from persistent storage.
// Ensures the clock starts up with correct color and brightness.
inline void initDisplay() {
    ledState.begin();
    displaySettings.begin();
    const GridVariantInfo* info = getGridVariantInfo(displaySettings.getGridVariant());
    if (info) {
        logInfo(String("🧩 Grid variant: ") + info->label + " (" + info->key + ")");
    }
    logInfo("🟢 LED and display settings initialized");
}
