#pragma once
#include <WebServer.h>
#include <network.h>
#include <Update.h>
#include "sequence_controller.h"
#include "led_state.h"
#include "time_mapper.h"
#include "ota_updater.h"


// Verwijzing naar globale variabelen
extern WebServer server;
extern String logBuffer[];
extern int logIndex;
extern bool clockEnabled;

// Functie om alle routes te registreren
void setupWebRoutes() {
  // Hoofdpagina
  server.on("/", []() {
    extern String getDashboardHTML(String logContent);
    String logContent = "";
    int i = logIndex;
    for (int count = 0; count < LOG_BUFFER_SIZE; count++) {
      String line = logBuffer[i];
      if (line.length() > 0) logContent += line + "\n";
      i = (i + 1) % LOG_BUFFER_SIZE;
    }
    server.send(200, "text/html", getDashboardHTML(logContent));
  });

  // Log ophalen
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

  // Status opvragen
  server.on("/status", []() {
    server.send(200, "text/plain", clockEnabled ? "on" : "off");
  });

  // Aan/uit zetten
  server.on("/toggle", []() {
    String state = server.arg("state");
    clockEnabled = (state == "on");
    server.send(200, "text/plain", "OK");
  });
  
  // Device restart
  server.on("/restart", []() {
    logln("⚠️ Herstart via dashboard aangevraagd");
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
    delay(100);  // Kleine vertraging om de HTTP-respons te voltooien
    ESP.restart();
  });

  server.on("/resetwifi", []() {
    logln("⚠️ Reset WiFi via dashboard aangevraagd");
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
    delay(100);  // Kleine vertraging om de HTTP-respons te voltooien
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
    logln("✨ Startup sequence gestart via dashboard");
    extern StartupSequence startupSequence;
    startupSequence.start();
    server.send(200, "text/plain", "Startup sequence uitgevoerd");
  });
  
  server.on("/uploadFirmware", HTTP_POST, []() {
    // Start the firmware upload process
    HTTPUpload& upload = server.upload();
  
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if (!filename.endsWith(".bin")) {
        server.send(400, "text/plain", "Invalid file format");
        return;
      }
  
      // Start the firmware update process
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        server.send(500, "text/plain", "Failed to start firmware update");
        return;
      }
  
      logln("Starting firmware update...");
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
      // Write the uploaded data to flash memory
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        server.send(500, "text/plain", "Failed to write firmware data");
        return;
      }
    }
    else if (upload.status == UPLOAD_FILE_END) {
      // Finalize the firmware update
      if (Update.end(true)) {
        server.send(200, "text/plain", "Firmware update successful. Restarting...");
        delay(1000);
        ESP.restart();
      } else {
        server.send(500, "text/plain", "Firmware update failed");
      }
    }
  });

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
  
    // Toepassen op actieve leds
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      auto indices = get_led_indices_for_time(&timeinfo);
      showLeds(indices);  // gebruikt actuele kleur + nieuwe helderheid
    }
  
    server.send(200, "text/plain", "OK");
  });
  
}
