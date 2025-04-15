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
  
  
}
