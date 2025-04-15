#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"

const char* VERSION_URL = "https://raw.githubusercontent.com/ronkuijpers/Wordclock/main/version.txt";
const char* FIRMWARE_URL = "https://github.com/ronkuijpers/Wordclock/releases/latest/download/firmware.bin";

void checkForFirmwareUpdate() {
  Serial.println("[OTA] Controle op nieuwe firmware...");

  HTTPClient http;
  http.begin(VERSION_URL);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("[OTA] Fout bij ophalen versie (%d)\n", httpCode);
    http.end();
    return;
  }

  String remoteVersion = http.getString();
  remoteVersion.trim();
  Serial.printf("[OTA] Huidige versie: %s | Beschikbaar: %s\n", BUILD_VERSION, remoteVersion.c_str());

  if (remoteVersion != BUILD_VERSION) {
    Serial.println("[OTA] Nieuwe firmware gevonden, update wordt uitgevoerd...");
    http.end();

    http.begin(FIRMWARE_URL);
    int code = http.GET();
    if (code == 200) {
      int len = http.getSize();
      WiFiClient* stream = http.getStreamPtr();

      if (!Update.begin(len)) {
        Serial.println("[OTA] Update.begin() mislukt");
        return;
      }

      size_t written = Update.writeStream(*stream);
      if (written == len && Update.end() && Update.isFinished()) {
        Serial.println("[OTA] Update geslaagd. Herstart...");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("[OTA] Update mislukt");
      }

    } else {
      Serial.printf("[OTA] Kon firmware niet downloaden (%d)\n", code);
    }

    http.end();
  } else {
    Serial.println("[OTA] Firmware is up-to-date.");
    http.end();
  }
}
