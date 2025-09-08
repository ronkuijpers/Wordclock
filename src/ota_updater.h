#pragma once
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "fs_compat.h"
#include "config.h"
#include "log.h"
#include "secrets.h"

static const char* FS_VERSION_FILE = "/.fs_version"; // marker

struct FileEntry {
  String path;
  String url;
  String sha256; // optioneel
};

static bool ensureDirs(const String& path) {
  // maakt tussenliggende directories aan
  for (size_t i = 1; i < path.length(); ++i) {
    if (path[i] == '/') {
      if (!FS_IMPL.mkdir(path.substring(0, i))) {
        // mkdir faalt vaak omdat 't al bestaat; dat is ok
      }
    }
  }
  return true;
}

// optionele sha256-verificatie placeholder; return true als je (nog) niet verifieert
static bool verifySha256(const String& /*expected*/, File& /*f*/) {
  // Implementatie kan met mbedTLS of externe lib; voor nu overslaan
  return true;
}

static bool downloadToFs(const String& url, const String& path, WiFiClientSecure& client) {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code != 200) {
    logError("HTTP " + String(code) + " for " + url);
    http.end();
    return false;
  }

  int len = http.getSize();
  if (len == 0) { http.end(); return false; }

  String tmp = path + ".tmp";
  ensureDirs(path);
  File f = FS_IMPL.open(tmp, "w");
  if (!f) { http.end(); return false; }

  WiFiClient& s = http.getStream();
  uint8_t buf[2048];
  int written = 0;
  while (http.connected() && (len > 0 || len == -1)) {
    size_t n = s.readBytes(buf, sizeof(buf));
    if (n == 0) break;
    f.write(buf, n);
    written += n;
    if (len > 0) len -= n;
  }
  f.flush(); f.close();
  http.end();

  // Atomic replace
  FS_IMPL.remove(path);
  if (!FS_IMPL.rename(tmp, path)) {
    FS_IMPL.remove(tmp);
    return false;
  }
  logInfo("Wrote " + path + " (" + String(written) + " bytes)");
  return true;
}

static String readFsVersion() {
  File f = FS_IMPL.open(FS_VERSION_FILE, "r");
  if (!f) return "";
  String v = f.readString();
  f.close();
  v.trim();
  return v;
}

static void writeFsVersion(const String& v) {
  File f = FS_IMPL.open(FS_VERSION_FILE, "w");
  if (!f) return;
  f.print(v);
  f.close();
}

static bool fetchManifest(JsonDocument& doc, WiFiClientSecure& client) {
  HTTPClient http;
  http.begin(client, VERSION_URL);
  int code = http.GET();
  if (code != 200) {
    logError("Failed to GET manifest: HTTP " + String(code));
    http.end();
    return false;
  }
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) { logError("JSON parse error"); return false; }
  return true;
}

static bool parseFiles(const JsonVariant& jfiles, std::vector<FileEntry>& out) {
  if (!jfiles.is<JsonArray>()) return false;
  for (JsonVariant v : jfiles.as<JsonArray>()) {
    FileEntry e;
    e.path = v["path"] | "";
    e.url  = v["url"]  | "";
    e.sha256 = v["sha256"] | "";
    if (e.path.length() && e.url.length()) out.push_back(e);
  }
  return true;
}

// === Public API ===

void syncFilesFromManifest() {
  logInfo("🔍 Checking UI files…");
  if (!FS_IMPL.begin(true)) {
    logError("FS mount failed");
    return;
  }

  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();

  JsonDocument doc; // ArduinoJson v7: dynamic capacity
  if (!fetchManifest(doc, *client)) return;

  // Prefer ui_version when present, else fall back to top-level version.
  String manifestVersion = doc["ui_version"].is<const char*>() ? String(doc["ui_version"].as<const char*>())
                          : (doc["version"].is<const char*>() ? String(doc["version"].as<const char*>()) : String(""));
  const String currentFsVer = readFsVersion();

  // Als we dezelfde versie al gesynct hebben, klaar (idempotent)
  if (manifestVersion.length() && manifestVersion == currentFsVer && doc["files"].isNull()) {
    logInfo("FS up-to-date (no files list).");
    return;
  }

  std::vector<FileEntry> files;
  if (doc["files"].is<JsonArray>() && parseFiles(doc["files"], files) && !files.empty()) {
    bool ok = true;
    for (const auto& e : files) {
      if (!downloadToFs(e.url, e.path, *client)) { ok = false; }
      // sha256 verify kan hier (optioneel)
      // if (ok && e.sha256.length()) { ... }
    }
    if (ok && manifestVersion.length()) writeFsVersion(manifestVersion);
    logInfo(ok ? "✅ UI files synced." : "⚠️ Some UI files failed.");
  } else {
    logInfo("No file list in manifest; skipping FS sync.");
  }
}

void checkForFirmwareUpdate() {
  logInfo("🔍 Checking for new firmware...");

  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();

  JsonDocument doc; // ArduinoJson v7
  if (!fetchManifest(doc, *client)) return;

  // Backward-compatible: firmware may be string or object
  String remoteVersion = doc["firmware"]["version"].is<const char*>() ? String(doc["firmware"]["version"].as<const char*>())
                       : (doc["version"].is<const char*>() ? String(doc["version"].as<const char*>()) : String(""));
  String fwUrl = doc["firmware"].is<const char*>() ? String(doc["firmware"].as<const char*>())
               : (doc["firmware"]["url"].is<const char*>() ? String(doc["firmware"]["url"].as<const char*>())
               : "");

  if (!fwUrl.length()) {
    logError("❌ Firmware URL missing");
    return;
  }

  logInfo("ℹ️ Remote version: " + remoteVersion);
  if (remoteVersion == FIRMWARE_VERSION) {
    logInfo("✅ Firmware already latest (" + String(FIRMWARE_VERSION) + ")");
    // We zitten al goed qua firmware; sync UI files toch nog
    syncFilesFromManifest();
    return;
  }

  // ---- OTA firmware ----
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, fwUrl)) {
    logError("❌ http.begin failed");
    return;
  }
  int code = http.GET();
  if (code != 200) {
    logError("❌ Firmware download failed: HTTP " + String(code));
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    logError("❌ Invalid firmware size");
    http.end();
    return;
  }

  if (!Update.begin(contentLength)) {
    logError("❌ Update.begin() failed");
    http.end();
    return;
  }

  WiFiClient& stream = http.getStream();
  size_t written = Update.writeStream(stream);
  http.end();

  if (written != (size_t)contentLength) {
    logError("❌ Incomplete write: " + String(written) + "/" + String(contentLength));
    Update.abort();
    return;
  }
  if (!Update.end()) {
    logError("❌ Update.end() failed");
    return;
  }
  if (Update.isFinished()) {
    logInfo("✅ Firmware updated, rebooting...");
    delay(500);
    ESP.restart();
  } else {
    logError("❌ Update not finished");
  }
}
