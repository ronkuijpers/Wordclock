#pragma once
#include <WebServer.h>

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
}
