#include "network.h"
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiServer.h>
#include "secrets.h"
#include "log.h"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void setupNetwork() {
  WiFiManager wm;
  bool res = wm.autoConnect(AP_NAME);
  if (!res) {
    logln("Kon geen verbinding maken!");
    ESP.restart();
  }
  logln("WiFi verbonden!");
  logln("IP-adres: " + WiFi.localIP().toString());

  // OTA
  ArduinoOTA.setHostname(CLOCK_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.onStart([]() { logln("Start OTA-update"); });
  ArduinoOTA.onEnd([]() { logln("Einde OTA-update"); });
  ArduinoOTA.begin();
  logln("OTA klaar!");

  // Telnet
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  logln("Telnet-server gestart op poort 23");
}

void handleTelnet() {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      telnetClient = telnetServer.available();
      logln("Nieuwe Telnet client verbonden");
      telnetClient.println("✅ Verbonden met Wordclock Telnet log");
    } else {
      WiFiClient extraClient = telnetServer.available();
      extraClient.stop();
    }
  }
}
