#pragma once

#include <time.h>
#include "log.h"

extern bool g_initialTimeSyncSucceeded;

// Initialize time synchronization via NTP
// This function sets the timezone and NTP servers, and waits until the time is successfully retrieved.
// Provides status messages via logging.
inline void initTimeSync(const char* tzInfo, const char* ntp1, const char* ntp2) {
    g_initialTimeSyncSucceeded = false;
    configTzTime(tzInfo, ntp1, ntp2); // Set timezone and NTP servers
    logInfo("⌛ Waiting for NTP...");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) { // Wait until time is available
        logDebug(".");
        delay(500);
    }
    logInfo("🕒 Time synchronized: " +
        String(timeinfo.tm_mday) + "/" +
        String(timeinfo.tm_mon+1) + " " +
        String(timeinfo.tm_hour) + ":" +
        String(timeinfo.tm_min));
    g_initialTimeSyncSucceeded = true;
}
