#include "webserver_init.h"
#include "mqtt_init.h"
#include "display_init.h"
#include "startup_sequence_init.h"
#include "wordclock_main.h"
#include "time_sync.h"
#include "wordclock_system_init.h"

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
#include "web_routes.h"
#include "network_init.h"
#include "log.h"
#include "config.h"
#include "ota_init.h"
#include "sequence_controller.h"
#include "display_settings.h"
#include "ui_auth.h"
#include "mqtt_client.h"
#include "night_mode.h"


bool clockEnabled = true;
StartupSequence startupSequence;
DisplaySettings displaySettings;
UiAuth uiAuth;
bool g_wifiHadCredentialsAtBoot = false;
static bool g_mqttInitialized = false;
static bool g_autoUpdateHandled = false;


// Webserver
WebServer server(80);

// Tracking (handled inside loop as statics)

// Setup: initialiseert hardware, netwerk, OTA, filesystem en start de hoofdservices
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  delay(MDNS_START_DELAY_MS);
  initLogSettings();

  initNetwork();              // WiFiManager (WiFi-instellingen en verbinding)
  initOTA();                  // OTA (Over-the-air updates)

  // Start mDNS voor lokale netwerknaam
  if (MDNS.begin(MDNS_HOSTNAME)) {
    logInfo("🌐 mDNS active at http://" MDNS_HOSTNAME ".local");
  } else {
    logError("❌ mDNS start failed");
  }

  // Load persisted display settings (e.g. auto-update preference) before running dependent flows
  displaySettings.begin();
  nightMode.begin();

  // Mount SPIFFS filesystem
  if (!FS_IMPL.begin(true)) {
  logError("SPIFFS mount failed.");
  } else {
  logDebug("SPIFFS loaded successfully.");
  logEnableFileSink();
  }

  initWebServer(server);      // Webserver en routes

  bool wifiConnected = isWiFiConnected();
  if (wifiConnected) {
    initMqtt();
    g_mqttInitialized = true;
    if (displaySettings.getAutoUpdate()) {
      logInfo("✅ Connected to WiFi. Starting firmware check...");
      checkForFirmwareUpdate();
    } else {
      logInfo("ℹ️ Automatic firmware updates disabled. Skipping check.");
    }
    g_autoUpdateHandled = true;
  } else {
    logInfo("⚠️ No WiFi. Waiting for connection or config portal.");
    g_autoUpdateHandled = !displaySettings.getAutoUpdate();
  }

  // Synchroniseer tijd via NTP
  initTimeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  initDisplay();
  initWordclockSystem(uiAuth);
  startupSequence.setWordWalkEnabled(!g_wifiHadCredentialsAtBoot);
  initStartupSequence(startupSequence);
}

// Loop: hoofdprogramma, verwerkt webrequests, OTA, MQTT en kloklogica
void loop() {
  processNetwork();
  if (isWiFiConnected() && !g_mqttInitialized) {
    initMqtt();
    g_mqttInitialized = true;
  }
  if (isWiFiConnected() && !g_autoUpdateHandled) {
    if (displaySettings.getAutoUpdate()) {
      logInfo("✅ Connected to WiFi. Starting firmware check...");
      checkForFirmwareUpdate();
    } else {
      logInfo("ℹ️ Automatic firmware updates disabled. Skipping check.");
    }
    g_autoUpdateHandled = true;
  }
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
          logInfo("🛠️ Daily firmware check started...");
          checkForFirmwareUpdate();
        } else {
          logInfo("ℹ️ Automatic firmware updates disabled (02:00 check skipped)");
        }
        lastFirmwareCheck = nowEpoch;
      }
    }
  }
}
