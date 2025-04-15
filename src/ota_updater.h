#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"
#include "log.h"

const char* VERSION_URL = "https://raw.githubusercontent.com/ronkuijpers/Wordclock/main/version.txt";
const char* FIRMWARE_URL = "https://github.com/ronkuijpers/Wordclock/releases/latest/download/firmware.bin";

void checkForFirmwareUpdate() {
  logln("[OTA] Controle op nieuwe firmware...");

  if (WiFi.status() != WL_CONNECTED) {
    logln("[OTA] Geen WiFi-verbinding, update afgebroken.");
    return;
  }

  // 🔐 Maak een veilige verbinding die geen certificaten eist
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();  // accepteert elk certificaat (voor GitHub HTTPS)

  HTTPClient http;
  http.begin(*client, VERSION_URL);

  int httpCode = http.GET();
  if (httpCode != 200) {
    logln("[OTA] Fout bij ophalen versie: " + String(httpCode));
    http.end();
    return;
  }

  String remoteVersion = http.getString();
  remoteVersion.trim();
  logln("[OTA] Huidige versie: " + String(BUILD_VERSION) + " | Beschikbaar: " + remoteVersion);

  http.end();  // sluit versie-check

  if (remoteVersion != BUILD_VERSION) {
    logln("[OTA] Nieuwe firmware gevonden, update wordt uitgevoerd...");

    // 🔁 Hergebruik een nieuwe client voor de download
    WiFiClientSecure *firmwareClient = new WiFiClientSecure;
    firmwareClient->setInsecure();

    HTTPClient firmwareHttp;
    firmwareHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // 🔁 automatisch redirects volgen
    firmwareHttp.begin(*firmwareClient, FIRMWARE_URL);


    int code = firmwareHttp.GET();
    if (code == 200) {
      int len = firmwareHttp.getSize();
      WiFiClient* stream = firmwareHttp.getStreamPtr();

      if (!Update.begin(len)) {
        logln("[OTA] Update.begin() mislukt");
        return;
      }

      size_t written = Update.writeStream(*stream);
      if (written == len && Update.end() && Update.isFinished()) {
        logln("[OTA] Update geslaagd. Herstart...");
        delay(1000);
        ESP.restart();
      } else {
        logln("[OTA] Update mislukt tijdens schrijven of afronden");
      }

    } else {
      logln("[OTA] Kon firmware niet downloaden. HTTP code: " + String(code));
    }

    firmwareHttp.end();
  } else {
    logln("[OTA] Firmware is up-to-date.");
  }
}
