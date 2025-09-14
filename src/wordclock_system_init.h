#pragma once

#include "ui_auth.h"
#include "wordclock.h"
#include "log.h"

// Initialiseer UI authenticatie en wordclock setup
inline void initWordclockSystem(UiAuth& uiAuth) {
    uiAuth.begin(UI_DEFAULT_PASS);
    wordclock_setup();
    logInfo("🟢 Wordclock systeem geinitialiseerd");
}
