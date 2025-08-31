#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "config.h"
#include "log.h"
#include "secrets.h"

void checkForFirmwareUpdate() {
  logInfo("🔍 Checking for new firmware...");

  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    logError("❌ Failed to allocate WiFiClientSecure");
    return;
  }

  client->setInsecure(); // Skip cert validation

  HTTPClient http;
  http.begin(*client, VERSION_URL);
  int httpCode = http.GET();

  if (httpCode != 200) {
    logError("❌ Failed to fetch firmware info: HTTP " + String(httpCode));
    http.end();
    delete client;
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error) {
    logError("❌ Failed to parse firmware info JSON");
    http.end();
    delete client;
    return;
  }

  if (!doc["firmware"].is<const char*>()) {
    logError("❌ Firmware URL missing or invalid in JSON");
    http.end();
    delete client;
    return;
  }

  String remoteVersion = doc["version"].as<String>();
  String firmwareUrl = doc["firmware"].as<String>();

  logInfo("ℹ️ Remote version: " + remoteVersion);

  if (remoteVersion == FIRMWARE_VERSION) {
    logInfo("✅ Already on the latest version (" + String(FIRMWARE_VERSION) + ")");
    http.end();
    delete client;
    return;
  }

  logInfo("⬇️ Firmware update available: " + firmwareUrl);
  http.end();

  HTTPClient firmwareHttp;
  firmwareHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  firmwareHttp.begin(*client, firmwareUrl);
  int firmwareCode = firmwareHttp.GET();

  if (firmwareCode != 200) {
    logError("❌ Firmware download failed: HTTP " + String(firmwareCode));
    firmwareHttp.end();
    delete client;
    return;
  }

  int contentLength = firmwareHttp.getSize();
  if (contentLength <= 0) {
    logError("❌ Invalid firmware size");
    firmwareHttp.end();
    delete client;
    return;
  }

  if (!Update.begin(contentLength)) {
    logError("❌ Update.begin() failed");
    firmwareHttp.end();
    delete client;
    return;
  }

  WiFiClient &stream = firmwareHttp.getStream();
  size_t written = Update.writeStream(stream);

  if (written != contentLength) {
    logError("❌ Incomplete write: " + String(written) + "/" + String(contentLength));
    Update.abort();
    firmwareHttp.end();
    delete client;
    return;
  }

  if (!Update.end()) {
    logError("❌ Update.end() failed");
    firmwareHttp.end();
    delete client;
    return;
  }

  if (Update.isFinished()) {
    logInfo("✅ Update successful, rebooting...");
    firmwareHttp.end();
    delete client;
    delay(1000);
    ESP.restart();
  } else {
    logError("❌ Update not finished");
    firmwareHttp.end();
    delete client;
  }
}
