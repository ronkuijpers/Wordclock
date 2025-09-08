#pragma once
#include <network.h>
#include "fs_compat.h"
#include <Update.h>
#include <WebServer.h>
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


// References to global variables
extern WebServer server;
extern String logBuffer[];
extern int logIndex;
extern bool clockEnabled;

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

// Function to register all routes
void setupWebRoutes() {
  // Main pages
  // Dashboard (protected)
  server.on("/dashboard.html", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    File f = FS_IMPL.open("/dashboard.html", "r");
    if (!f) { server.send(404, "text/plain", "dashboard not found"); return; }
    server.streamFile(f, "text/html");
    f.close();
  });

  // Forgot password page (public)
  server.on("/forgot.html", HTTP_GET, []() {
    File f = FS_IMPL.open("/forgot.html", "r");
    if (!f) { server.send(404, "text/plain", "forgot not found"); return; }
    server.streamFile(f, "text/html");
    f.close();
  });

  // Factory reset (public, requires POST). Resets preferences + WiFi and restarts.
  server.on("/factoryreset", HTTP_POST, []() {
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head><meta http-equiv='refresh' content='8;url=/' /></head>
        <body>
          <h1>Factory reset gestart...</h1>
          <p>Het apparaat wordt teruggezet naar fabrieksinstellingen en herstart zo.</p>
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
    File f = FS_IMPL.open("/changepw.html", "r");
    if (!f) { server.send(404, "text/plain", "changepw not found"); return; }
    server.streamFile(f, "text/html");
    f.close();
  });

  // Logout endpoints: return 401 to clear Basic Auth in browser
  server.on("/logout", HTTP_GET, []() {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"Wordclock UI\"");
    server.send(401, "text/plain", "Uitgelogd. Sluit het tabblad of log opnieuw in.");
  });
  server.on("/adminlogout", HTTP_GET, []() {
    server.sendHeader("WWW-Authenticate", String("Basic realm=\"") + ADMIN_REALM + "\"");
    server.send(401, "text/plain", "Admin uitgelogd. Sluit het tabblad of log opnieuw in.");
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
    if (n != c) { server.send(400, "text/plain", "Wachtwoorden komen niet overeen"); return; }
    if (n.length() < 6) { server.send(400, "text/plain", "Minimaal 6 tekens"); return; }
    if (!uiAuth.setPassword(n)) { server.send(500, "text/plain", "Opslaan mislukt"); return; }
    server.send(200, "text/plain", "OK");
  });

  // Protected admin page (Admin auth only)
  server.on("/admin.html", HTTP_GET, []() {
    if (!ensureAdminAuth()) return;
    File f = FS_IMPL.open("/admin.html", "r");
    if (!f) {
      server.send(404, "text/plain", "admin.html not found");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
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
    server.streamFile(f, "text/html");
    f.close();
  });

  // Update page (protected)
  server.on("/update.html", HTTP_GET, []() {
    if (!ensureUiAuth()) return;
    File f = FS_IMPL.open("/update.html", "r");
    if (!f) { server.send(404, "text/plain", "update not found"); return; }
    server.streamFile(f, "text/html");
    f.close();
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
    logInfo("⚠️ Herstart via dashboard aangevraagd");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='5;url=/' />
        </head>
        <body>
          <h1>Wordclock wordt herstart...</h1>
          <p>Je wordt over 5 seconden automatisch teruggestuurd naar het dashboard.</p>
        </body>
      </html>
    )rawliteral");
    delay(100);  // Small delay to finish the HTTP response
    ESP.restart();
  });

  server.on("/resetwifi", []() {
    if (!ensureUiAuth()) return;
    logInfo("⚠️ Reset WiFi via dashboard aangevraagd");
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta http-equiv='refresh' content='10;url=/' />
        </head>
        <body>
          <h1>WiFi wordt gereset...</h1>
          <p>WiFi-instellingen worden gewist. Mogelijk moet je opnieuw verbinding maken met het access point 'Wordclock'.</p>
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
    logInfo("✨ Startup sequence gestart via dashboard");
    extern StartupSequence startupSequence;
    startupSequence.start();
    server.send(200, "text/plain", "Startup sequence uitgevoerd");
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
        logInfo("📂 Start upload: " + upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          logError("❌ Update.begin() mislukt");
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          logError("❌ Fout bij schrijven chunk");
          Update.printError(Serial);
        } else {
          logDebug("✏️ Geschreven: " + String(written) + " bytes");
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        logInfo("📥 Upload voltooid");
        logDebug("Totaal " + String(Update.size()) + " bytes");
        if (!Update.end(true)) {
          logError("❌ Update.end() mislukt");
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
        logInfo("📂 Start SPIFFS upload: " + upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
          logError("❌ Update.begin(U_SPIFFS) mislukt");
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          logError("❌ Fout bij schrijven chunk (SPIFFS)");
          Update.printError(Serial);
        } else {
          logDebug("✏️ SPIFFS geschreven: " + String(written) + " bytes");
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        logInfo("📥 SPIFFS upload voltooid");
        logDebug("SPIFFS totaal " + String(Update.size()) + " bytes");
        if (!Update.end(true)) {
          logError("❌ Update.end(U_SPIFFS) mislukt");
          Update.printError(Serial);
        }
      }
    }
  );

  server.on("/checkForUpdate", HTTP_ANY, []() {
    if (!ensureUiAuth()) return;
    logInfo("Firmware update handmatig gestart via UI");
    server.send(200, "text/plain", "Firmware update gestart");
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
    logInfo(String("🛒 Verkooptijd ") + (on ? "AAN (10:47)" : "UIT"));
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
    logInfo(String("🎞️ Animatie ") + (on ? "AAN" : "UIT"));
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
    logInfo("⏱️ HET IS duur ingesteld op " + String(val) + "s");
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
    logInfo("🔧 Log level gewijzigd naar: " + levelStr);
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
