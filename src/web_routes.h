#pragma once
#include <network.h>
#include <SPIFFS.h>
#include <Update.h>
#include <WebServer.h>
#include "sequence_controller.h"
#include "led_state.h"
#include "log.h"
#include "time_mapper.h"
#include "ota_updater.h"
#include "led_controller.h"
#include "config.h"
#include "display_settings.h"


// References to global variables
extern WebServer server;
extern String logBuffer[];
extern int logIndex;
extern bool clockEnabled;

// Function to register all routes
void setupWebRoutes() {
  // Main pages
  server.serveStatic("/dashboard.html", SPIFFS, "/dashboard2.html", "max-age=86400");
  // Legacy assets no longer used; keep route mapping simple

  server.serveStatic("/dashboard2.html", SPIFFS, "/dashboard2.html", "max-age=86400");

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Location", "/dashboard2.html", true);
    server.send(302, "text/plain", "");
  });

  // Fetch log
  server.on("/log", []() {
    String logContent = "";
    int i = logIndex;
    for (int count = 0; count < LOG_BUFFER_SIZE; count++) {
      String line = logBuffer[i];
      if (line.length() > 0) logContent += line + "\n";
      i = (i + 1) % LOG_BUFFER_SIZE;
    }
    server.send(200, "text/plain", logContent);
  });

  // Get status
  server.on("/status", []() {
    server.send(200, "text/plain", clockEnabled ? "on" : "off");
  });

  // Turn on/off
  server.on("/toggle", []() {
    String state = server.arg("state");
    clockEnabled = (state == "on");
    // Apply immediately
    if (clockEnabled) {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        auto indices = get_led_indices_for_time(&timeinfo);
        showLeds(indices);
      }
    } else {
      // Clear LEDs when turning off
      showLeds({});
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Device restart
  server.on("/restart", []() {
    logInfo("⚠️ Herstart via dashboard aangevraagd");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='5;url=/' />
        </head>
        <body>
          <h1>Wordclock wordt herstart...</h1>
          <p>Je wordt over 5 seconden automatisch teruggestuurd naar het dashboard.</p>
        </body>
      </html>
    )rawliteral");
    delay(100);  // Small delay to finish the HTTP response
    ESP.restart();
  });

  server.on("/resetwifi", []() {
    logInfo("⚠️ Reset WiFi via dashboard aangevraagd");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='10;url=/' />
        </head>
        <body>
          <h1>WiFi wordt gereset...</h1>
          <p>WiFi-instellingen worden gewist. Mogelijk moet je opnieuw verbinding maken met het access point 'Wordclock'.</p>
        </body>
      </html>
    )rawliteral");
    delay(100);  // Small delay to finish the HTTP response
    resetWiFiSettings();
  });
  
  server.on("/setColor", HTTP_GET, []() {
    if (!server.hasArg("color")) {
      server.send(400, "text/plain", "Missing color");
      return;
    }
    String hex = server.arg("color");  // "RRGGBB"
    if (hex.length() != 6) {
      server.send(400, "text/plain", "Invalid color");
      return;
    }
  
    long val = strtol(hex.c_str(), nullptr, 16);
    uint8_t r = (val >> 16) & 0xFF;
    uint8_t g = (val >> 8) & 0xFF;
    uint8_t b =  val       & 0xFF;
  
    ledState.setRGB(r, g, b);
  
    // Refresh display immediately with new color
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      std::vector<uint16_t> indices = get_led_indices_for_time(&timeinfo);
      showLeds(indices);
    }
  
    server.send(200, "text/plain", "OK");
  });
  
  
  server.on("/startSequence", []() {
    logInfo("✨ Startup sequence gestart via dashboard");
    extern StartupSequence startupSequence;
    startupSequence.start();
    server.send(200, "text/plain", "Startup sequence uitgevoerd");
  });
  
  server.on(
    "/uploadFirmware",
    HTTP_POST,
    []() {
      server.send(200, "text/plain", Update.hasError() ? "Firmware update failed" : "Firmware update successful. Rebooting...");
      if (!Update.hasError()) {
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      HTTPUpload& upload = server.upload();
  
      if (upload.status == UPLOAD_FILE_START) {
        logInfo("📂 Start upload: " + upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          logError("❌ Update.begin() mislukt");
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          logError("❌ Fout bij schrijven chunk");
          Update.printError(Serial);
        } else {
          logDebug("✏️ Geschreven: " + String(written) + " bytes");
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        logInfo("📥 Upload voltooid");
        logDebug("Totaal " + String(Update.size()) + " bytes");
        if (!Update.end(true)) {
          logError("❌ Update.end() mislukt");
          Update.printError(Serial);
        }
      }
    }
  );  

  server.on("/checkForUpdate", HTTP_ANY, []() {
    server.send(200, "text/plain", "Firmware update gestart");
    delay(100);
    checkForFirmwareUpdate();
  });

  server.on("/getBrightness", []() {
    server.send(200, "text/plain", String(ledState.getBrightness()));
  });  

  server.on("/setBrightness", []() {
    if (!server.hasArg("level")) {
      server.send(400, "text/plain", "Missing brightness level");
      return;
    }
  
    int level = server.arg("level").toInt();
    level = constrain(level, 0, 255);
    ledState.setBrightness(level);
  
      // Apply to active LEDs
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      auto indices = get_led_indices_for_time(&timeinfo);
      showLeds(indices);  // uses current color + new brightness
    }
  
    server.send(200, "text/plain", "OK");
  });

  // Expose firmware version
  server.on("/version", []() {
    server.send(200, "text/plain", FIRMWARE_VERSION);
  });

  // Het Is duration (0..360 seconds; 0=never, 360=always)
  server.on("/getHetIsDuration", []() {
    server.send(200, "text/plain", String(displaySettings.getHetIsDurationSec()));
  });

  server.on("/setHetIsDuration", []() {
    if (!server.hasArg("seconds")) {
      server.send(400, "text/plain", "Missing seconds");
      return;
    }
    int val = server.arg("seconds").toInt();
    if (val < 0) val = 0; if (val > 360) val = 360;
    displaySettings.setHetIsDurationSec((uint16_t)val);
    logInfo("⏱️ HET IS duur ingesteld op " + String(val) + "s");
    server.send(200, "text/plain", "OK");
  });

  server.on("/setLogLevel", HTTP_ANY, []() {
    if (!server.hasArg("level")) {
      server.send(400, "text/plain", "Missing log level");
      return;
    }

    String levelStr = server.arg("level");
    LogLevel level;

    if (levelStr == "DEBUG") level = LOG_LEVEL_DEBUG;
    else if (levelStr == "INFO") level = LOG_LEVEL_INFO;
    else if (levelStr == "WARN") level = LOG_LEVEL_WARN;
    else if (levelStr == "ERROR") level = LOG_LEVEL_ERROR;
    else {
      server.send(400, "text/plain", "Invalid log level");
      return;
    }


    setLogLevel(level);
    logInfo("🔧 Log level gewijzigd naar: " + levelStr);
    server.send(200, "text/plain", "OK");
  });

  server.on("/getLogLevel", HTTP_GET, []() {
    // Return current level as string
    String s = "INFO";
    extern LogLevel LOG_LEVEL; // declared in log.cpp
    switch (LOG_LEVEL) {
      case LOG_LEVEL_DEBUG: s = "DEBUG"; break;
      case LOG_LEVEL_INFO:  s = "INFO";  break;
      case LOG_LEVEL_WARN:  s = "WARN";  break;
      case LOG_LEVEL_ERROR: s = "ERROR"; break;
    }
    server.send(200, "text/plain", s);
  });

  
}
