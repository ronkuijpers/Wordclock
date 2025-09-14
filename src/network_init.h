#pragma once

#include <WiFi.h>
#include <WiFiManager.h>
#include "log.h"
#include "secrets.h"

// Initialiseer WiFi-verbinding via WiFiManager
inline void initNetwork() {
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);  // Sluit AP na 3 minuten
    logInfo("WiFiManager start verbinding...");
    bool res = wm.autoConnect(AP_NAME, AP_PASSWORD);
    if (!res) {
        logError("❌ Geen WiFi-verbinding. Herstart...");
        ESP.restart();
    }
    logInfo("✅ WiFi verbonden met netwerk: " + String(WiFi.SSID()));
    logInfo("📡 IP-adres: " + WiFi.localIP().toString());
}
