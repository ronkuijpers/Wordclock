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
  logln("🔍 Checking for new firmware...");

  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    logln("❌ Failed to allocate WiFiClientSecure");
    return;
  }

  client->setInsecure(); // Accept all certificates (useful for self-signed)

  HTTPClient firmwareHttp;
  firmwareHttp.begin(*client, OTA_UPDATE_URL);

  int httpCode = firmwareHttp.GET();
  if (httpCode != HTTP_CODE_OK) {
    logln("❌ Firmware check failed: HTTP " + String(httpCode));
    firmwareHttp.end();
    delete client;
    return;
  }

  int contentLength = firmwareHttp.getSize();
  if (contentLength <= 0) {
    logln("❌ Invalid firmware size");
    firmwareHttp.end();
    delete client;
    return;
  }

  logln("⬇️ Starting OTA update (" + String(contentLength) + " bytes)");

  if (!Update.begin(contentLength)) {
    logln("❌ Update.begin() failed");
    firmwareHttp.end();
    delete client;
    return;
  }

  WiFiClient& stream = firmwareHttp.getStream();
  size_t written = Update.writeStream(stream);

  if (written != contentLength) {
    logln("❌ Incomplete write: " + String(written) + "/" + String(contentLength));
    Update.abort();
    firmwareHttp.end();
    delete client;
    return;
  }

  if (!Update.end()) {
    logln("❌ Update.end() failed");
    firmwareHttp.end();
    delete client;
    return;
  }

  if (Update.isFinished()) {
    logln("✅ Update successful, rebooting...");
    firmwareHttp.end();
    delete client;
    delay(1000);
    ESP.restart();
  } else {
    logln("❌ Update not finished");
    firmwareHttp.end();
    delete client;
  }
}


// void checkForFirmwareUpdate() {
//   logln("[OTA] Controle op nieuwe firmware...");

//   if (WiFi.status() != WL_CONNECTED) {
//     logln("[OTA] Geen WiFi-verbinding, update afgebroken.");
//     return;
//   }

//   // 🔐 Maak een veilige verbinding die geen certificaten eist
//   WiFiClientSecure *client = new WiFiClientSecure;
//   client->setInsecure();  // accepteert elk certificaat (voor GitHub HTTPS)

//   HTTPClient http;
//   String versionUrl = String(VERSION_URL) + "?ts=" + String(millis());
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
//   http.begin(*client, versionUrl);
//   http.addHeader("Cache-Control", "no-cache");
//   http.addHeader("Pragma", "no-cache");


//   int httpCode = http.GET();
//   if (httpCode != 200) {
//     logln("[OTA] Fout bij ophalen versie: " + String(httpCode));
//     http.end();
//     return;
//   }

//   String remoteVersion = http.getString();
//   remoteVersion.trim();
//   logln("[OTA] Huidige versie: " + String(BUILD_VERSION) + " | Beschikbaar: " + remoteVersion);

//   http.end();  // sluit versie-check

//   if (remoteVersion != BUILD_VERSION) {
//     logln("[OTA] Nieuwe firmware gevonden, update wordt uitgevoerd...");

//     // 🔁 Hergebruik een nieuwe client voor de download
//     WiFiClientSecure *firmwareClient = new WiFiClientSecure;
//     firmwareClient->setInsecure();

//     HTTPClient firmwareHttp;
//     firmwareHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // 🔁 automatisch redirects volgen
//     firmwareHttp.begin(*firmwareClient, FIRMWARE_URL);


//     int code = firmwareHttp.GET();
//     if (code == 200) {
//       int len = firmwareHttp.getSize();
//       WiFiClient* stream = firmwareHttp.getStreamPtr();

//       if (!Update.begin(len)) {
//         logln("[OTA] Update.begin() mislukt");
//         return;
//       }

//       size_t written = Update.writeStream(*stream);
//       if (written == len && Update.end() && Update.isFinished()) {
//         logln("[OTA] Update geslaagd. Herstart...");
//         delay(1000);
//         ESP.restart();
//       } else {
//         logln("[OTA] Update mislukt tijdens schrijven of afronden");
//       }

//     } else {
//       logln("[OTA] Kon firmware niet downloaden. HTTP code: " + String(code));
//     }

//     firmwareHttp.end();
//   } else {
//     logln("[OTA] Firmware is up-to-date.");
//   }
// }
