#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <WiFiServer.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "wordclock.h"
#include "secrets.h"
#include "esp_log.h"
#include "dashboard_html.h"
#include "web_routes.h"
#include "network.h"
#include "log.h"
#include "config.h"
#include "ota_updater.h"

bool clockEnabled = true;

// Webserver
WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);

  checkForFirmwareUpdate();  // Bij startup

  setupNetwork();  // bevat WiFiManager, OTA en Telnet-setup

  // Setup Dashboard
  setupWebRoutes();
  server.begin();

  // Tijd sync
  configTime(0, 0, NTP_SERVER, BACKUP_NTP_SERVER);
  logln("Wachten op tijd via NTP...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    log(".");
    delay(500);
  }
  logln("Tijd gesynchroniseerd!");

  wordclock_setup();
}

unsigned long lastUpdate = 0;
time_t lastFirmwareCheck = 0;

void loop() {
  handleTelnet();
  server.handleClient();
  ArduinoOTA.handle();

  // Wordclock update elke seconde
  unsigned long now = millis();
  if (now - lastUpdate > 1000) {
    wordclock_loop();
    lastUpdate = now;
  }

  // Firmware update om 02:00 uur
  time_t currentTime = time(nullptr);
  struct tm timeinfo;
  if (localtime_r(&currentTime, &timeinfo)) {
    if (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && currentTime - lastFirmwareCheck > 3600) {
      Serial.println("[Loop] Dagelijkse firmwarecheck wordt uitgevoerd...");
      checkForFirmwareUpdate();
      lastFirmwareCheck = currentTime;
    }
  }

  // Eventueel delay van 10-50 ms als je CPU-belasting wil beperken
  delay(50);
}

