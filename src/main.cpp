#include <Arduino.h>
#include <ESPmDNS.h>
#include "fs_compat.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <WiFiServer.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "wordclock.h"
#include "secrets.h"
#include "web_routes.h"
#include "network.h"
#include "log.h"
#include "config.h"
#include "ota_updater.h"
#include "sequence_controller.h"
#include "display_settings.h"
#include "ui_auth.h"
#include "mqtt_client.h"


bool clockEnabled = true;
StartupSequence startupSequence;
DisplaySettings displaySettings;
UiAuth uiAuth;


// Webserver
WebServer server(80);

// Tracking (handled inside loop as statics)

void setup() {
  Serial.begin(115200);
  delay(1000);
  initLogSettings();

  setupNetwork();             // WiFiManager
  setupOTA();                 // OTA
  // setupTelnet();              // Telnet

  if (MDNS.begin("wordclock")) {
    logInfo("🌐 mDNS actief op http://wordclock.local");
  } else {
    logError("❌ mDNS start mislukt");
  }

  if (!FS_IMPL.begin(true)) {
    logError("SPIFFS mounten mislukt.");
  } else {
    logDebug("SPIFFS succesvol geladen.");
  }

  setupWebRoutes();           // Dashboard-routes
  server.begin();

  logInfo("Controleren op WiFi verbinding");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    logInfo(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    mqtt_begin();
    if (displaySettings.getAutoUpdate()) {
      logInfo("✅ Verbonden met WiFi. Start firmwarecheck...");
      checkForFirmwareUpdate();
    } else {
      logInfo("ℹ️ Automatische firmwareupdates uitgeschakeld. Sla check over.");
    }
  } else {
    logInfo("⚠️ Geen WiFi. Firmwarecheck overgeslagen.");
  }

  // Synchronize time via NTP
  configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
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

  ledState.begin();
  displaySettings.begin();
  uiAuth.begin(UI_DEFAULT_PASS);
  // initWordMap() no longer needed; static lookup is used
  wordclock_setup();

  startupSequence.start();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  mqtt_loop();

  // Startup animatie
  if (startupSequence.isRunning()) {
    startupSequence.update();
    return;  // <-- Voorkomt dat klok al tijd toont
  }

  // Tijd- en animatie-update (wordclock_loop regelt zelf per-minuut/animatie)
  static unsigned long lastLoop = 0;
  unsigned long now = millis();
  if (now - lastLoop >= 50) {
    lastLoop = now;
    wordclock_loop();

    // Dagelijkse firmwarecheck om 02:00
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      time_t nowEpoch = time(nullptr);
      static time_t lastFirmwareCheck = 0;
      if (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && nowEpoch - lastFirmwareCheck > 3600) {
        if (displaySettings.getAutoUpdate()) {
          logInfo("🛠️ Dagelijkse firmwarecheck gestart...");
          checkForFirmwareUpdate();
        } else {
          logInfo("ℹ️ Automatische firmwareupdates uitgeschakeld (02:00 check overgeslagen)");
        }
        lastFirmwareCheck = nowEpoch;
      }
    }
  }
}
