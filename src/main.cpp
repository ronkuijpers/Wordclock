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

bool clockEnabled = true;

// Webserver
WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);

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
void loop() {
  handleTelnet();
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long now = millis();
  if (now - lastUpdate > 1000) {
    wordclock_loop();
    lastUpdate = now;
  }
}
