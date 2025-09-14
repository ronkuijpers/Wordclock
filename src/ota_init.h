#pragma once

#include <ArduinoOTA.h>
#include "log.h"
#include "secrets.h"

// Initialiseer OTA (Over-the-air updates)
inline void initOTA() {
    ArduinoOTA.setHostname(CLOCK_NAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        logInfo("🔄 Start netwerk OTA-update");
    });
    ArduinoOTA.onEnd([]() {
        logInfo("✅ OTA-update voltooid, restart in 1s");
        delay(1000);
        ESP.restart();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        uint8_t pct = (progress * 100) / total;
        logInfo("📶 OTA Progress: " + String(pct) + "%");
    });
    ArduinoOTA.onError([](ota_error_t err) {
        String msg = "[OTA] Fout: ";
        switch (err) {
            case OTA_AUTH_ERROR:    msg += "Authenticatie mislukt"; break;
            case OTA_BEGIN_ERROR:   msg += "Begin mislukt";       break;
            case OTA_CONNECT_ERROR: msg += "Connectie mislukt";   break;
            case OTA_RECEIVE_ERROR: msg += "Ontvang mislukt";     break;
            case OTA_END_ERROR:     msg += "Eind mislukt";        break;
            default:                msg += "Onbekend";            break;
        }
        logError(msg);
    });
    ArduinoOTA.begin();
    logInfo("🟢 Netwerk OTA-service actief, wacht op upload");
}
