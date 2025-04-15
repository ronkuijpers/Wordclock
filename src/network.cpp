#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiServer.h>
#include "secrets.h"
#include "log.h"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void setupNetwork() {
  WiFiManager wm;
  logln("WiFiManager start verbinding...");
  bool res = wm.autoConnect(AP_NAME);

  if (!res) {
    logln("❌ Geen WiFi-verbinding. Herstart...");
    ESP.restart();
  }

  logln("✅ WiFi verbonden met netwerk: " + String(WiFi.SSID()));
  logln("📡 IP-adres: " + WiFi.localIP().toString());

  // OTA via LAN
  ArduinoOTA.setHostname(CLOCK_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.onStart([]() { logln("🔄 Start netwerk OTA-update"); });
  ArduinoOTA.onEnd([]() { logln("✅ OTA-update voltooid"); });
  ArduinoOTA.begin();
  logln("🟢 Netwerk OTA-service actief, wacht op upload");

  // Telnet
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  logln("✅ Telnet-server gestart op poort 23");
}

void handleTelnet() {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      telnetClient = telnetServer.available();
      logln("🔌 Nieuwe Telnet-client verbonden op " + telnetClient.remoteIP().toString());
      telnetClient.println("✅ Verbonden met Wordclock Telnet log");
    } else {
      WiFiClient extraClient = telnetServer.available();
      extraClient.stop(); // Alleen één client tegelijk
    }
  }
}
