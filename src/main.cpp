#include "webserver_init.h"
#include "mqtt_init.h"
#include "display_init.h"
#include "startup_sequence_init.h"
#include "wordclock_main.h"
#include "time_sync.h"
#include "wordclock_system_init.h"
// TODO: Overweeg dit bestand te splitsen in modules zoals netwerk, OTA, tijdsynchronisatie en hoofdlogica voor betere onderhoudbaarheid.

// Wordclock hoofdprogramma
// - Setup: initialiseert hardware, netwerk, OTA, filesystem en start services
// - Loop: verwerkt webrequests, OTA, MQTT en kloklogica

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
#include "network_init.h"
#include "log.h"
#include "config.h"
#include "ota_init.h"
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

// Setup: initialiseert hardware, netwerk, OTA, filesystem en start de hoofdservices
void setup() {
  Serial.begin(115200);
  delay(1000);
  initLogSettings();

  initNetwork();              // WiFiManager (WiFi-instellingen en verbinding)
  initOTA();                  // OTA (Over-the-air updates)
  // setupTelnet();           // Telnet logging (optioneel)

  // Start mDNS voor lokale netwerknaam
  if (MDNS.begin("wordclock")) {
    logInfo("🌐 mDNS actief op http://wordclock.local");
  } else {
    logError("❌ mDNS start mislukt");
  }

  // Mount SPIFFS filesystem
  if (!FS_IMPL.begin(true)) {
    logError("SPIFFS mounten mislukt.");
  } else {
    logDebug("SPIFFS succesvol geladen.");
  }

  initWebServer(server);      // Webserver en routes

  // Wacht op WiFi verbinding (max 20x proberen)
  logInfo("Controleren op WiFi verbinding");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    logInfo(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    initMqtt();
    if (displaySettings.getAutoUpdate()) {
      logInfo("✅ Verbonden met WiFi. Start firmwarecheck...");
      checkForFirmwareUpdate();
    } else {
      logInfo("ℹ️ Automatische firmwareupdates uitgeschakeld. Sla check over.");
    }
  } else {
    logInfo("⚠️ Geen WiFi. Firmwarecheck overgeslagen.");
  }

  // Synchroniseer tijd via NTP
  initTimeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  initDisplay();
  initWordclockSystem(uiAuth);
  initStartupSequence(startupSequence);
}

// Loop: hoofdprogramma, verwerkt webrequests, OTA, MQTT en kloklogica
void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  mqttEventLoop();

  // Startup animatie: blokkeert klok tot animatie klaar is
  if (updateStartupSequence(startupSequence)) {
    return;  // Voorkomt dat klok al tijd toont
  }

  // Tijd- en animatie-update (wordclock_loop regelt zelf per-minuut/animatie)
  static unsigned long lastLoop = 0;
  unsigned long now = millis();
  if (now - lastLoop >= 50) {
    lastLoop = now;
    runWordclockLoop();

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
