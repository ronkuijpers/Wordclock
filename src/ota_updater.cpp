#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "fs_compat.h"
#include "config.h"
#include "log.h"
#include "secrets.h"
#include "ota_updater.h"
#include "display_settings.h"

static const char* FS_VERSION_FILE = "/.fs_version"; // marker

struct FileEntry {
  String path;
  String url;
  String sha256; // optioneel
};

static bool ensureDirs(const String& path) {
  for (size_t i = 1; i < path.length(); ++i) {
    if (path[i] == '/') {
      if (!FS_IMPL.mkdir(path.substring(0, i))) {
        // ok if exists
      }
    }
  }
  return true;
}

static bool verifySha256(const String& /*expected*/, File& /*f*/) {
  return true;
}

static bool selectFirmware(const JsonDocument& doc, const String& channel, String& outVersion, String& outUrl) {
  outVersion = "";
  outUrl = "";
  const char* block = (channel == "early") ? "early" : "firmware";
  JsonVariantConst fw = doc[block];
  if (fw.is<JsonObject>()) {
    if (fw["version"].is<const char*>()) outVersion = String(fw["version"].as<const char*>());
    if (fw["url"].is<const char*>()) outUrl = String(fw["url"].as<const char*>());
  } else if (fw.is<const char*>()) {
    outUrl = String(fw.as<const char*>());
  }
  if (outUrl.length() == 0 && channel == "early") {
    fw = doc["firmware"];
    if (fw.is<JsonObject>()) {
      if (fw["version"].is<const char*>()) outVersion = String(fw["version"].as<const char*>());
      if (fw["url"].is<const char*>()) outUrl = String(fw["url"].as<const char*>());
    } else if (fw.is<const char*>()) {
      outUrl = String(fw.as<const char*>());
    }
  }
  return outUrl.length() > 0;
}

static bool downloadToFs(const String& url, const String& path, WiFiClientSecure& client) {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
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
  http.setTimeout(15000);
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

void syncFilesFromManifest() {
  logInfo("🔍 Checking UI files…");
  if (!FS_IMPL.begin(true)) {
    logError("FS mount failed");
    return;
  }

  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();

  JsonDocument doc;
  if (!fetchManifest(doc, *client)) return;

  String manifestVersion = doc["ui_version"].is<const char*>() ? String(doc["ui_version"].as<const char*>())
                          : (doc["version"].is<const char*>() ? String(doc["version"].as<const char*>()) : String(""));
  const String currentFsVer = readFsVersion();

  if (manifestVersion.length() && manifestVersion == currentFsVer) {
    logInfo("UI up-to-date (version match).");
    return;
  }

  std::vector<FileEntry> files;
  if (doc["files"].is<JsonArray>() && parseFiles(doc["files"], files) && !files.empty()) {
    bool ok = true;
    for (const auto& e : files) {
      if (!downloadToFs(e.url, e.path, *client)) { ok = false; }
    }
    if (ok && manifestVersion.length()) writeFsVersion(manifestVersion);
    logInfo(ok ? "✅ UI files synced." : "⚠️ Some UI files failed.");
  } else {
    logInfo("No file list in manifest; skipping UI sync.");
  }
}

void checkForFirmwareUpdate() {
  logInfo("🔍 Checking for new firmware...");

  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();

  JsonDocument doc;
  if (!fetchManifest(doc, *client)) return;

  String remoteVersion;
  String fwUrl;
  String channel = displaySettings.getUpdateChannel();
  if (!selectFirmware(doc, channel, remoteVersion, fwUrl)) {
    logError("❌ Firmware URL missing");
    return;
  }

  logInfo("ℹ️ Remote version (" + channel + "): " + remoteVersion);
  if (remoteVersion == FIRMWARE_VERSION) {
    logInfo("✅ Firmware already latest (" + String(FIRMWARE_VERSION) + ")");
    syncFilesFromManifest();
    return;
  }

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
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
