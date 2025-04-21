#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiServer.h>
#include "secrets.h"
#include "log.h"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  wm.setConfigPortalTimeout(180);  // optioneel: sluit AP na 3 minuten

  logln("WiFiManager start verbinding...");
  bool res = wm.autoConnect(AP_NAME, AP_PASSWORD);

  if (!res) {
    logln("❌ Geen WiFi-verbinding. Herstart...");
    ESP.restart();
  }

  logln("✅ WiFi verbonden met netwerk: " + String(WiFi.SSID()));
  logln("📡 IP-adres: " + WiFi.localIP().toString());
}


void setupOTA() {
  // 1) Basisconfiguratie
  ArduinoOTA.setHostname(CLOCK_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);

  // 2) Callbacks voor extra logging
  ArduinoOTA.onStart([]() {
    logln("🔄 Start netwerk OTA-update");
  });
  ArduinoOTA.onEnd([]() {
    logln("✅ OTA-update voltooid, restart in 1s");
    delay(1000);
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // percent = (progress/total)*100
    uint8_t pct = (progress * 100) / total;
    logln("📶 OTA Progress: " + String(pct) + "%");
  });
  ArduinoOTA.onError([](ota_error_t err) {
    String msg = "[OTA] Fout: ";
    switch (err) {
      case OTA_AUTH_ERROR:    msg += "Authenticatie mislukt"; break;
      case OTA_BEGIN_ERROR:   msg += "Begin mislukt";       break;
      case OTA_CONNECT_ERROR: msg += "Connectie mislukt";   break;
      case OTA_RECEIVE_ERROR: msg += "Ontvang mislukt";     break;
      case OTA_END_ERROR:     msg += "Eind mislukt";        break;
      default:                msg += "Onbekend";            break;
    }
    logln(msg);
  });

  // 3) Start de service
  ArduinoOTA.begin();
  logln("🟢 Netwerk OTA-service actief, wacht op upload");
}

void setupTelnet() {
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

void resetWiFiSettings() {
  logln("🔁 WiFiManager instellingen worden gewist...");
  WiFiManager wm;
  wm.resetSettings();     // <-- belangrijk
  delay(500);             // geef de EEPROM even tijd
  ESP.restart();
}
