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
#include "sequence_controller.h"


bool clockEnabled = true;
StartupSequence startupSequence;


// Webserver
WebServer server(80);

// Tracking
time_t lastFirmwareCheck = 0;
int lastDisplayedMinute = -1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupNetwork();             // WiFiManager
  setupOTA();                 // OTA
  // setupTelnet();              // Telnet
  setupWebRoutes();           // Dashboard-routes
  server.begin();

  logln("Controleren op WiFi verbinding");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    logln(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    logln("✅ Verbonden met WiFi. Start firmwarecheck...");
    checkForFirmwareUpdate();
  } else {
    logln("⚠️ Geen WiFi. Firmwarecheck overgeslagen.");
  }

  // Synchronize time via NTP
  configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  logln("⌛ Wachten op NTP...");
  
  struct tm timeinfo;
  
  while (!getLocalTime(&timeinfo)) {
    log(".");
    delay(500);
  }
  logln("🕒 Tijd gesynchroniseerd: " +
    String(timeinfo.tm_mday) + "/" +
    String(timeinfo.tm_mon+1) + " " +
    String(timeinfo.tm_hour) + ":" +
    String(timeinfo.tm_min));

  ledState.begin();
  initWordMap();
  wordclock_setup();
}

void loop() {
  static unsigned long lastLoop = 0;
  unsigned long now = millis();
  if (now - lastLoop >= 50) {
    lastLoop = now;

    // Update only if the minute has changed
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_min != lastDisplayedMinute) {
        char buf[20];  // enough space for emoji + " HH:MM\0"
        snprintf(buf, sizeof(buf), "🔄 %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        logln(String(buf));
        wordclock_loop();
        lastDisplayedMinute = timeinfo.tm_min;
      }

      // Daily firmware check at 02:00
      time_t nowEpoch = time(nullptr);
      if (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && nowEpoch - lastFirmwareCheck > 3600) {
        logln("🛠️ Dagelijkse firmwarecheck gestart...");
        checkForFirmwareUpdate();
        lastFirmwareCheck = nowEpoch;
      }
    }
  }

  // handleTelnet();
  server.handleClient();
  ArduinoOTA.handle();

  // Non-blocking startup animation step
  if (startupSequence.isRunning()) {
    startupSequence.update();
  }

}
