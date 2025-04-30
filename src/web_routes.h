#pragma once
#include <WebServer.h>
#include <network.h>
#include "sequence_controller.h"


// Verwijzing naar globale variabelen
extern WebServer server;
extern String logBuffer[];
extern int logIndex;
extern bool clockEnabled;
uint8_t currentR = 0, currentG = 0, currentB = 0, currentW = 255;

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
          <meta http-equiv='refresh' content='10;url=/' />
        </head>
        <body>
          <h1>Wordclock wordt herstart...</h1>
          <p>Je wordt over 10 seconden automatisch teruggestuurd naar het dashboard.</p>
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
  
  server.on("/setColor", HTTP_GET, [&]() {
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
    // Haal RGB uit de hex
    currentR = (val >> 16) & 0xFF;
    currentG = (val >> 8)  & 0xFF;
    currentB =  val        & 0xFF;

    if (currentR==255 && currentG==255 && currentB==255) {
      currentW = 255;
      currentR = currentG = currentB = 0;
    } else {
      currentW = 0;
    }

    server.send(200, "text/plain", "OK");
  });
  
  server.on("/startSequence", []() {
    logln("✨ Startup sequence gestart via dashboard");
    startupSequence();
    server.send(200, "text/plain", "Startup sequence uitgevoerd");
  });
  
  
}
