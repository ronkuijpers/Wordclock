#pragma once

#include <time.h>
#include "log.h"

// Initialiseer tijdsynchronisatie via NTP
inline void initTimeSync(const char* tzInfo, const char* ntp1, const char* ntp2) {
    configTzTime(tzInfo, ntp1, ntp2);
    logInfo("⌛ Wachten op NTP...");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        logDebug(".");
        delay(500);
    }
    logInfo("🕒 Tijd gesynchroniseerd: " +
        String(timeinfo.tm_mday) + "/" +
        String(timeinfo.tm_mon+1) + " " +
        String(timeinfo.tm_hour) + ":" +
        String(timeinfo.tm_min));
}
