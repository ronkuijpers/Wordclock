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

// Tracking
time_t lastFirmwareCheck = 0;
int lastDisplayedMinute = -1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupNetwork();             // WiFiManager
  setupOTA();                 // OTA
  setupTelnet();              // Telnet
  setupWebRoutes();           // Dashboard-routes
  server.begin();

  logln("Controleren op WiFi verbinding");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    logln(".");
    retry++;
  }

  // weer uncommenten als aan de gang met OTA
  // if (WiFi.status() == WL_CONNECTED) {
  //   logln("✅ Verbonden met WiFi. Start firmwarecheck...");
  //   checkForFirmwareUpdate();
  // } else {
  //   logln("⚠️ Geen WiFi. Firmwarecheck overgeslagen.");
  // }

  // Tijd synchroniseren via NTP
  // CET-1CEST,M3.5.0/2,M10.5.0/3 betekent:
  //   Winter: CET = UTC+1  (CE[T]-1)
  //   Zomer:  CEST = UTC+2 (CEST)
  //   Wissel op de laatste zondag van maart om 02:00 en oktober om 03:00
  configTime("CET-1CEST,M3.5.0/2,M10.5.0/3", NTP_SERVER, BACKUP_NTP_SERVER);
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

  wordclock_setup();
}

void loop() {
  handleTelnet();
  server.handleClient();
  ArduinoOTA.handle();

  // Alleen updaten als de minuut veranderd is
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_min != lastDisplayedMinute) {
      char buf[20];  // ruim genoeg voor emoji + " HH:MM\0"
      snprintf(buf, sizeof(buf), "🔄 %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      logln(String(buf));
      wordclock_loop();
      lastDisplayedMinute = timeinfo.tm_min;
    }

    // Dagelijkse firmwarecheck om 02:00
    // time_t now = time(nullptr);
    // if (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && now - lastFirmwareCheck > 3600) {
    //   logln("🛠️ Dagelijkse firmwarecheck gestart...");
    //   checkForFirmwareUpdate();
    //   lastFirmwareCheck = now;
    // }
  }

  delay(50); // Laat ruimte over voor andere processen
}
