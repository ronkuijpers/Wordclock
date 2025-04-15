#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"

const char* VERSION_URL = "https://raw.githubusercontent.com/ronkuijpers/Wordclock/main/version.txt";
const char* FIRMWARE_URL = "https://github.com/ronkuijpers/Wordclock/releases/latest/download/firmware.bin";

void checkForFirmwareUpdate() {
  logln("[OTA] Controle op nieuwe firmware...");

  HTTPClient http;
  http.begin(VERSION_URL);
  int httpCode = http.GET();

  if (httpCode != 200) {
    logln("[OTA] Fout bij ophalen versie (%d)\n", httpCode);
    http.end();
    return;
  }

  String remoteVersion = http.getString();
  remoteVersion.trim();
  logln("[OTA] Huidige versie: " + String(BUILD_VERSION) + " | Beschikbaar: " + remoteVersion);


  if (remoteVersion != BUILD_VERSION) {
    logln("[OTA] Nieuwe firmware gevonden, update wordt uitgevoerd...");
    http.end();

    http.begin(FIRMWARE_URL);
    int code = http.GET();
    if (code == 200) {
      int len = http.getSize();
      WiFiClient* stream = http.getStreamPtr();

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
        logln("[OTA] Update mislukt");
      }

    } else {
      logln("[OTA] Kon firmware niet downloaden (%d)\n", code);
    }

    http.end();
  } else {
    logln("[OTA] Firmware is up-to-date.");
    http.end();
  }
}
