#pragma once
#include <network.h>
#include "fs_compat.h"
#include <Update.h>
#include <WebServer.h>
#include <esp_system.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "sequence_controller.h"
#include "led_state.h"
#include "log.h"
#include "time_mapper.h"
#include "ota_updater.h"
#include "led_controller.h"
#include "config.h"
#include "display_settings.h"
#include "ui_auth.h"
#include "wordclock.h"
#include "mqtt_settings.h"
#include "mqtt_client.h"


// References to global variables
extern WebServer server;
extern String logBuffer[];
extern int logIndex;
extern bool clockEnabled;

// Serve file, preferring a .gz variant if client accepts gzip
static void serveFile(const char* path, const char* mime) {
  // Gzip temporarily disabled; always serve plain files
  bool acceptGzip = false;
  String gzPath = String(path) + ".gz";
  if (acceptGzip) {
    File gz = FS_IMPL.open(gzPath, "r");
    if (gz) {
      server.sendHeader("Content-Encoding", "gzip");
      server.streamFile(gz, mime);
      gz.close();
      return;
    }
  }
  File f = FS_IMPL.open(path, "r");
  if (!f) { server.send(404, "text/plain", String(path) + " not found"); return; }
  server.streamFile(f, mime);
  f.close();
}
// Simple Basic-Auth guard for admin resources
static bool ensureAdminAuth() {
  if (!server.authenticate(ADMIN_USER, ADMIN_PASS)) {
    server.requestAuthentication(BASIC_AUTH, ADMIN_REALM);
    return false;
  }
  return true;
}

// Guard for general UI access (dynamic password via Preferences)
static bool ensureUiAuth() {
  // Allow admin credentials to pass UI guard as well
  if (server.authenticate(ADMIN_USER, ADMIN_PASS)) {
    return true;
  }
  // Basic Auth with dynamic UI creds
  if (!server.authenticate(uiAuth.getUser().c_str(), uiAuth.getPass().c_str())) {
    server.requestAuthentication(BASIC_AUTH, "Wordclock UI");
    return false;
  }
  // Force password change flow (only for UI user, not admin)
  if (uiAuth.needsChange()) {
    String uri = server.uri();
    if (!(uri == "/changepw.html" || uri == "/setUIPassword")) {
      server.sendHeader("Location", "/changepw.html", true);
      server.send(302, "text/plain", "");
      return false;
    }
  }
  return true;
}

// Clear persistent settings (factory reset helper)
static void performFactoryReset() {
  Preferences p;
  const char* keys[] = { "ui_auth", "display", "led", "log" };
  for (auto ns : keys) {
    p.begin(ns, false);
    p.clear();
    p.end();
  }
}

// Token for allowing factory reset from Forgot Password page
static String g_factoryToken;
static unsigned long g_factoryTokenExp = 0; // millis deadline

static String generateFactoryToken(unsigned long ttl_ms = 60000) {
  char buf[24];
  uint32_t r = esp_random();
  snprintf(buf, sizeof(buf), "%08X%08lX", (unsigned)r, (unsigned long)millis());
  g_factoryToken = String(buf);
  g_factoryTokenExp = millis() + ttl_ms;
  return g_factoryToken;
}

// Function to register all routes
void setupWebRoutes() {
  // Capture Accept-Encoding so we can serve gzip if available
  static const char* headerKeys[] = { "Accept-Encoding" };
  server.collectHeaders(headerKeys, 1);

  // Helper defined at file scope: serveFile()
  // Main pages
  // Dashboard (protected)
  server.on("/dashboard.html", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    serveFile("/dashboard.html", "text/html");
  });

  // Forgot password page (public)
  server.on("/forgot.html", HTTP_GET, []() {
    serveFile("/forgot.html", "text/html");
  });

  // Factory reset token endpoint (public): returns a short-lived token for reset
  server.on("/factorytoken", HTTP_GET, []() {
    // Issue new token valid for 60s
    String tok = generateFactoryToken(60000);
    server.send(200, "text/plain", tok);
  });

  // Factory reset (admin or valid token, requires POST). Resets preferences + WiFi and restarts.
  server.on("/factoryreset", HTTP_POST, []() {
    bool allowed = false;
    // Admin credentials allow reset unconditionally
    if (server.authenticate(ADMIN_USER, ADMIN_PASS)) {
      allowed = true;
    } else {
      // Check for valid short-lived token
      if (server.hasArg("token")) {
        String tok = server.arg("token");
        if (tok.length() > 0 && tok == g_factoryToken) {
          unsigned long now = millis();
          if (g_factoryTokenExp != 0 && (long)(g_factoryTokenExp - now) > 0) {
            allowed = true;
          }
        }
      }
    }
    if (!allowed) {
      // Prefer admin auth prompt for browsers; otherwise 403 if token supplied but invalid
      if (!server.hasArg("token")) {
        server.requestAuthentication(BASIC_AUTH, ADMIN_REALM);
      } else {
        server.send(403, "text/plain", "Forbidden");
      }
      return;
    }
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head><meta http-equiv='refresh' content='8;url=/' /></head>
        <body>
          <h1>Factory reset started...</h1>
          <p>The device will reset to factory defaults and reboot shortly.</p>
        </body>
      </html>
    )rawliteral");
    delay(200);
    performFactoryReset();
    resetWiFiSettings(); // will restart
  });

  // Change password page (protected, but accessible during forced-change flow)
  server.on("/changepw.html", HTTP_GET, []() {
    // Only require Basic Auth, no redirect to itself
    if (!server.authenticate(uiAuth.getUser().c_str(), uiAuth.getPass().c_str())) {
      server.requestAuthentication(BASIC_AUTH, "Wordclock UI");
      return;
    }
    serveFile("/changepw.html", "text/html");
  });

  // Logout endpoints: return 401 to clear Basic Auth in browser
  server.on("/logout", HTTP_GET, []() {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"Wordclock UI\"");
    server.send(401, "text/plain", "Logged out. Close the tab or log in again.");
  });
  server.on("/adminlogout", HTTP_GET, []() {
    server.sendHeader("WWW-Authenticate", String("Basic realm=\"") + ADMIN_REALM + "\"");
    server.send(401, "text/plain", "Admin logged out. Close the tab or log in again.");
  });

  // Handle password change
  server.on("/setUIPassword", HTTP_POST, []() {
    if (!server.authenticate(uiAuth.getUser().c_str(), uiAuth.getPass().c_str())) {
      server.requestAuthentication(BASIC_AUTH, "Wordclock UI");
      return;
    }
    if (!server.hasArg("new") || !server.hasArg("confirm")) {
      server.send(400, "text/plain", "Missing fields");
      return;
    }
    String n = server.arg("new");
    String c = server.arg("confirm");
    if (n != c) { server.send(400, "text/plain", "Passwords do not match"); return; }
    if (n.length() < 6) { server.send(400, "text/plain", "Minimum 6 characters"); return; }
    if (!uiAuth.setPassword(n)) { server.send(500, "text/plain", "Save failed"); return; }
    server.send(200, "text/plain", "OK");
  });

  // Protected admin page (Admin auth only)
  server.on("/admin.html", HTTP_GET, []() {
    if (!ensureAdminAuth()) return;
    serveFile("/admin.html", "text/html");
  });

  // Public landing page with links to Login (protected) and Forgot
  server.on("/", HTTP_GET, []() {
    File f = FS_IMPL.open("/login.html", "r");
    if (!f) {
      // Fallback: redirect to protected dashboard
      server.sendHeader("Location", "/dashboard.html", true);
      server.send(302, "text/plain", "");
      return;
    }
    f.close();
    serveFile("/login.html", "text/html");
  });

  // Update page (protected)
  server.on("/update.html", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    serveFile("/update.html", "text/html");
  });

  // MQTT settings page (protected)
  server.on("/mqtt.html", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    serveFile("/mqtt.html", "text/html");
  });

  // MQTT config API
  server.on("/api/mqtt/config", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    MqttSettings cfg;
    mqtt_settings_load(cfg);
    // Do not expose password; indicate if set
    String json = "{";
    json += "\"host\":\"" + cfg.host + "\",";
    json += "\"port\":" + String(cfg.port) + ",";
    json += "\"user\":\"" + cfg.user + "\",";
    json += "\"has_pass\":" + String(cfg.pass.length() > 0 ? "true" : "false") + ",";
    json += "\"discovery\":\"" + cfg.discoveryPrefix + "\",";
    json += "\"base\":\"" + cfg.baseTopic + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/mqtt/config", HTTP_POST, []() {
    if (!ensureUiAuth()) return;
    MqttSettings current;
    mqtt_settings_load(current);
    MqttSettings next = current;

    if (server.hasArg("host")) next.host = server.arg("host");
    if (server.hasArg("port")) next.port = (uint16_t) server.arg("port").toInt();
    if (server.hasArg("user")) next.user = server.arg("user");
    if (server.hasArg("pass")) {
      String p = server.arg("pass");
      if (p.length() > 0) next.pass = p; // empty means keep existing
    }
    if (server.hasArg("discovery")) next.discoveryPrefix = server.arg("discovery");
    if (server.hasArg("base")) next.baseTopic = server.arg("base");

    // Basic validation
    if (next.host.length() == 0 || next.port == 0) {
      server.send(400, "text/plain", "host/port required");
      return;
    }

    mqtt_apply_settings(next);
    server.send(200, "text/plain", "OK");
  });

  // MQTT runtime status
  server.on("/api/mqtt/status", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    bool c = mqtt_is_connected();
    String json = String("{\"connected\":") + (c ? "true" : "false") + 
                  ",\"last_error\":\"" + mqtt_last_error() + "\"}";
    server.send(200, "application/json", json);
  });

  // MQTT connection test (does not save). Accepts form-encoded: host, port, user?, pass?
  server.on("/api/mqtt/test", HTTP_POST, []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("host") || !server.hasArg("port")) {
      server.send(400, "text/plain", "host/port required");
      return;
    }
    String host = server.arg("host");
    uint16_t port = (uint16_t) server.arg("port").toInt();
    String user = server.arg("user");
    String pass = server.arg("pass");

    // Quick TCP reachability test
    WiFiClient testClient;
    testClient.setTimeout(3000);
    if (!testClient.connect(host.c_str(), port)) {
      server.send(502, "text/plain", "TCP connect failed");
      return;
    }
    testClient.stop();

    // Optional MQTT handshake if user provided
    if (user.length() > 0 || pass.length() > 0) {
      WiFiClient mc;
      PubSubClient tmp(mc);
      tmp.setServer(host.c_str(), port);
      String cid = String("wordclock_test_") + String(millis());
      bool ok = tmp.connect(cid.c_str(), user.c_str(), pass.c_str());
      if (!ok) {
        int st = tmp.state();
        server.send(401, "text/plain", String("MQTT auth failed (state ") + st + ")");
        return;
      }
      tmp.disconnect();
    }

    server.send(200, "text/plain", "OK");
  });

  // Auto update toggle
  server.on("/getAutoUpdate", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", displaySettings.getAutoUpdate() ? "on" : "off");
  });
  server.on("/setAutoUpdate", []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("state")) {
      server.send(400, "text/plain", "Missing state");
      return;
    }
    String st = server.arg("state");
    bool on = (st == "on" || st == "1" || st == "true");
    displaySettings.setAutoUpdate(on);
  logInfo(String("🔁 Auto firmware updates ") + (on ? "ON" : "OFF"));
    server.send(200, "text/plain", "OK");
  });

  // Fetch log
  server.on("/log", []() {
    if (!ensureUiAuth()) return;
    String logContent = "";
    int i = logIndex;
    for (int count = 0; count < LOG_BUFFER_SIZE; count++) {
      String line = logBuffer[i];
      if (line.length() > 0) logContent += line + "\n";
      i = (i + 1) % LOG_BUFFER_SIZE;
    }
    server.send(200, "text/plain", logContent);
  });

  // Get status
  server.on("/status", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", clockEnabled ? "on" : "off");
  });

  // Turn on/off
  server.on("/toggle", []() {
    if (!ensureUiAuth()) return;
    String state = server.arg("state");
    clockEnabled = (state == "on");
    // Apply immediately
    if (clockEnabled) {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        auto indices = get_led_indices_for_time(&timeinfo);
        showLeds(indices);
      }
    } else {
      // Clear LEDs when turning off
      showLeds({});
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Device restart
  server.on("/restart", []() {
    if (!ensureUiAuth()) return;
  logInfo("⚠️ Restart requested via dashboard");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='5;url=/' />
        </head>
        <body>
          <h1>Wordclock is restarting...</h1>
          <p>You will be redirected to the dashboard in 5 seconds.</p>
        </body>
      </html>
    )rawliteral");
    delay(100);  // Small delay to finish the HTTP response
    ESP.restart();
  });

  server.on("/resetwifi", []() {
    if (!ensureUiAuth()) return;
  logInfo("⚠️ WiFi reset requested via dashboard");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='10;url=/' />
        </head>
        <body>
          <h1>Resetting WiFi...</h1>
          <p>WiFi settings will be cleared. You may need to reconnect to the 'Wordclock' access point.</p>
        </body>
      </html>
    )rawliteral");
    delay(100);  // Small delay to finish the HTTP response
    resetWiFiSettings();
  });
  
  server.on("/setColor", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("color")) {
      server.send(400, "text/plain", "Missing color");
      return;
    }
    String hex = server.arg("color");  // "RRGGBB"
    if (hex.length() != 6) {
      server.send(400, "text/plain", "Invalid color");
      return;
    }
  
    long val = strtol(hex.c_str(), nullptr, 16);
    uint8_t r = (val >> 16) & 0xFF;
    uint8_t g = (val >> 8) & 0xFF;
    uint8_t b =  val       & 0xFF;
  
    ledState.setRGB(r, g, b);
  
    // Refresh display immediately with new color
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      std::vector<uint16_t> indices = get_led_indices_for_time(&timeinfo);
      showLeds(indices);
    }
  
    server.send(200, "text/plain", "OK");
  });

  // Get current color as RRGGBB (white maps to FFFFFF)
  server.on("/getColor", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    uint8_t r, g, b, w;
    ledState.getRGBW(r, g, b, w);
    if (w > 0) { r = g = b = 255; }
    char buf[7];
    snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
    server.send(200, "text/plain", String(buf));
  });
  
  
  server.on("/startSequence", []() {
    if (!ensureUiAuth()) return;
  logInfo("✨ Startup sequence started via dashboard");
    extern StartupSequence startupSequence;
    startupSequence.start();
    server.send(200, "text/plain", "Startup sequence executed");
  });
  
  server.on(
    "/uploadFirmware",
    HTTP_POST,
    []() {
      if (!ensureUiAuth()) return;
      server.send(200, "text/plain", Update.hasError() ? "Firmware update failed" : "Firmware update successful. Rebooting...");
      if (!Update.hasError()) {
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      if (!ensureUiAuth()) return;
      HTTPUpload& upload = server.upload();

      if (upload.status == UPLOAD_FILE_START) {
        logInfo("📂 Upload started: " + upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          logError("❌ Update.begin() failed");
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          logError("❌ Error writing chunk");
          Update.printError(Serial);
        } else {
          logDebug("✏️ Written: " + String(written) + " bytes");
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        logInfo("📥 Upload completed");
        logDebug("Total " + String(Update.size()) + " bytes");
        if (!Update.end(true)) {
          logError("❌ Update.end() failed");
          Update.printError(Serial);
        }
      }
    }
  );  

  // Separate endpoint for SPIFFS (UI) updates
  server.on(
    "/uploadSpiffs",
    HTTP_POST,
    []() {
      if (!ensureUiAuth()) return;
      server.send(200, "text/plain", Update.hasError() ? "SPIFFS update failed" : "SPIFFS update successful. Rebooting...");
      if (!Update.hasError()) {
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      if (!ensureUiAuth()) return;
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        logInfo("📂 SPIFFS upload started: " + upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
          logError("❌ Update.begin(U_SPIFFS) failed");
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          logError("❌ Error writing chunk (SPIFFS)");
          Update.printError(Serial);
        } else {
          logDebug("✏️ SPIFFS written: " + String(written) + " bytes");
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        logInfo("📥 SPIFFS upload completed");
        logDebug("SPIFFS total " + String(Update.size()) + " bytes");
        if (!Update.end(true)) {
          logError("❌ Update.end(U_SPIFFS) failed");
          Update.printError(Serial);
        }
      }
    }
  );

  server.on("/checkForUpdate", HTTP_ANY, []() {
    if (!ensureUiAuth()) return;
    logInfo("Firmware update manually started via UI");
    server.send(200, "text/plain", "Firmware update started");
    delay(100);
    checkForFirmwareUpdate();
  });

  server.on("/getBrightness", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", String(ledState.getBrightness()));
  });  

  server.on("/setBrightness", []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("level")) {
      server.send(400, "text/plain", "Missing brightness level");
      return;
    }
  
    int level = server.arg("level").toInt();
    level = constrain(level, 0, 255);
    ledState.setBrightness(level);
  
      // Apply to active LEDs
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      auto indices = get_led_indices_for_time(&timeinfo);
      showLeds(indices);  // uses current color + new brightness
    }
  
    server.send(200, "text/plain", "OK");
  });

  // Expose firmware version
  server.on("/version", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", FIRMWARE_VERSION);
  });

  // UI version from config
  server.on("/uiversion", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", UI_VERSION);
  });

  // Sell mode endpoints (force 10:47 display)
  server.on("/getSellMode", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", displaySettings.isSellMode() ? "on" : "off");
  });
  server.on("/setSellMode", []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("state")) {
      server.send(400, "text/plain", "Missing state");
      return;
    }
    String st = server.arg("state");
    bool on = (st == "on" || st == "1" || st == "true");
    displaySettings.setSellMode(on);
    // Trigger animation to new effective time
    struct tm t = {};
    if (on) {
      t.tm_hour = 10;
      t.tm_min = 47;
    } else {
      if (!getLocalTime(&t)) { server.send(200, "text/plain", "OK"); return; }
    }
    wordclock_force_animation_for_time(&t);
  logInfo(String("🛒 Sell time ") + (on ? "ON (10:47)" : "OFF"));
    server.send(200, "text/plain", "OK");
  });

  // Word-by-word animation toggle
  server.on("/getAnimate", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", displaySettings.getAnimateWords() ? "on" : "off");
  });
  server.on("/setAnimate", []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("state")) {
      server.send(400, "text/plain", "Missing state");
      return;
    }
    String st = server.arg("state");
    bool on = (st == "on" || st == "1" || st == "true");
    displaySettings.setAnimateWords(on);
  logInfo(String("🎞️ Animation ") + (on ? "ON" : "OFF"));
    server.send(200, "text/plain", "OK");
  });

  // Het Is duration (0..360 seconds; 0=never, 360=always)
  server.on("/getHetIsDuration", []() {
    if (!ensureUiAuth()) return;
    server.send(200, "text/plain", String(displaySettings.getHetIsDurationSec()));
  });

  server.on("/setHetIsDuration", []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("seconds")) {
      server.send(400, "text/plain", "Missing seconds");
      return;
    }
    int val = server.arg("seconds").toInt();
    if (val < 0) val = 0; if (val > 360) val = 360;
    displaySettings.setHetIsDurationSec((uint16_t)val);
  logInfo("⏱️ HET IS duration set to " + String(val) + "s");
    server.send(200, "text/plain", "OK");
  });

  server.on("/setLogLevel", HTTP_ANY, []() {
    if (!ensureUiAuth()) return;
    if (!server.hasArg("level")) {
      server.send(400, "text/plain", "Missing log level");
      return;
    }

    String levelStr = server.arg("level");
    LogLevel level;

    if (levelStr == "DEBUG") level = LOG_LEVEL_DEBUG;
    else if (levelStr == "INFO") level = LOG_LEVEL_INFO;
    else if (levelStr == "WARN") level = LOG_LEVEL_WARN;
    else if (levelStr == "ERROR") level = LOG_LEVEL_ERROR;
    else {
      server.send(400, "text/plain", "Invalid log level");
      return;
    }


    setLogLevel(level);
  logInfo("🔧 Log level changed to: " + levelStr);
    server.send(200, "text/plain", "OK");
  });

  server.on("/getLogLevel", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    // Return current level as string
    String s = "INFO";
    extern LogLevel LOG_LEVEL; // declared in log.cpp
    switch (LOG_LEVEL) {
      case LOG_LEVEL_DEBUG: s = "DEBUG"; break;
      case LOG_LEVEL_INFO:  s = "INFO";  break;
      case LOG_LEVEL_WARN:  s = "WARN";  break;
      case LOG_LEVEL_ERROR: s = "ERROR"; break;
    }
    server.send(200, "text/plain", s);
  });

  
}
