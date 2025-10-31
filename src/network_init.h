#pragma once

#include <WiFi.h>
#include <WiFiManager.h>
#include "log.h"
#include "secrets.h"

extern bool g_wifiHadCredentialsAtBoot;

// Initialize WiFi connection using WiFiManager
// This function starts WiFi in station mode, shows a config portal if needed,
// and ensures the device is connected to the network. On failure, the device will restart.
inline void initNetwork() {
    WiFi.mode(WIFI_STA); // Set WiFi to station mode
    WiFiManager wm;
    wm.setConfigPortalTimeout(WIFI_CONFIG_PORTAL_TIMEOUT);  // Close AP after WIFI_CONFIG_PORTAL_TIMEOUT seconds
    g_wifiHadCredentialsAtBoot = wm.getWiFiIsSaved();
    logInfo(String("WiFiManager starting connection (credentials present: ") + (g_wifiHadCredentialsAtBoot ? "yes" : "no") + ")");
    bool res = wm.autoConnect(AP_NAME, AP_PASSWORD); // Connect to WiFi or open portal
    if (!res) {
    logError("❌ No WiFi connection. Restarting...");
        ESP.restart();
    }
    logInfo("✅ WiFi connected to network: " + String(WiFi.SSID()));
    logInfo("📡 IP address: " + WiFi.localIP().toString());
}
