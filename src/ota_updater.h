#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "config.h"
#include "log.h"

void checkForFirmwareUpdate() {
  logln("🔍 Checking for new firmware...");

  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    logln("❌ Failed to allocate WiFiClientSecure");
    return;
  }

  client->setInsecure(); // Accept all certificates (for GitHub raw)

  HTTPClient versionHttp;
  versionHttp.begin(*client, VERSION_URL);

  int httpCode = versionHttp.GET();
  if (httpCode != HTTP_CODE_OK) {
    logln("❌ Version check failed: HTTP " + String(httpCode));
    versionHttp.end();
    delete client;
    return;
  }

  String payload = versionHttp.getString();
  versionHttp.end();

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    logln("❌ JSON parse failed: " + String(error.c_str()));
    delete client;
    return;
  }

  String remoteVersion = doc["version"] | "";
  String firmwareUrl = doc["url"] | FIRMWARE_URL;

  if (remoteVersion == "" || firmwareUrl == "") {
    logln("❌ Invalid version data in JSON");
    delete client;
    return;
  }

  if (remoteVersion == FIRMWARE_VERSION) {
    logln("✅ Firmware is up to date (v" + remoteVersion + ")");
    delete client;
    return;
  }

  logln("⬇️ New firmware v" + remoteVersion + " found. Starting OTA...");

  HTTPClient firmwareHttp;
  firmwareHttp.begin(*client, firmwareUrl);

  int fwCode = firmwareHttp.GET();
  if (fwCode != HTTP_CODE_OK) {
    logln("❌ Firmware download failed: HTTP " + String(fwCode));
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
