#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiServer.h>
#include "secrets.h"
#include "log.h"

// WiFiServer telnetServer(23);
// WiFiClient telnetClient;

void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  wm.setConfigPortalTimeout(WIFI_CONFIG_PORTAL_TIMEOUT);  // optional: close AP after WIFI_CONFIG_PORTAL_TIMEOUT seconds

  logInfo("WiFiManager starting connection...");
  bool res = wm.autoConnect(AP_NAME, AP_PASSWORD);

  if (!res) {
  logError("❌ No WiFi connection. Restarting...");
    ESP.restart();
  }

  logInfo("✅ WiFi connected to network: " + String(WiFi.SSID()));
  logInfo("📡 IP address: " + WiFi.localIP().toString());
}


void setupOTA() {
  // 1) Basic configuration
  ArduinoOTA.setHostname(CLOCK_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);

  // 2) Callbacks for additional logging
  ArduinoOTA.onStart([]() {
  logInfo("🔄 Starting network OTA update");
  });
  ArduinoOTA.onEnd([]() {
  logInfo("✅ OTA update complete, restarting in 1s");
    delay(OTA_UPDATE_COMPLETE_DELAY_MS);
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // percent = (progress/total)*100
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

  // 3) Start the service
  ArduinoOTA.begin();
  logInfo("🟢 Network OTA service active, waiting for upload");
}


void resetWiFiSettings() {
  logInfo("🔁 WiFiManager settings are being cleared...");
  WiFiManager wm;
  wm.resetSettings();     // <-- important
  delay(EEPROM_WRITE_DELAY_MS); // give the EEPROM some time
  ESP.restart();
}
