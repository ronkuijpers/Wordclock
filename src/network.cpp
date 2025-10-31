#include <WiFi.h>
#include <WiFiManager.h>

#include "network_init.h"
#include "config.h"
#include "log.h"
#include "led_controller.h"
#include "secrets.h"

extern bool clockEnabled;
extern bool g_wifiHadCredentialsAtBoot;

namespace {

WiFiManager& getManager() {
  static WiFiManager manager;
  return manager;
}

bool g_wifiConnected = false;

} // namespace

void initNetwork() {
  WiFi.mode(WIFI_STA);
  auto& wm = getManager();

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(WIFI_CONFIG_PORTAL_TIMEOUT);
  wm.setAPClientCheck(false);  // allow AP even when STA disconnected
  wm.setCleanConnect(true);    // ensure fresh STA connect attempts
  wm.setSTAStaticIPConfig(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
  wm.setDebugOutput(true, WM_DEBUG_DEV);

  g_wifiHadCredentialsAtBoot = wm.getWiFiIsSaved();
  logInfo(String("WiFiManager starting connection (credentials present: ") + (g_wifiHadCredentialsAtBoot ? "yes" : "no") + ")");

  bool autoResult = wm.autoConnect(AP_NAME, AP_PASSWORD);
  (void)autoResult;
  g_wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (g_wifiConnected) {
    logInfo("✅ WiFi connected to network: " + String(WiFi.SSID()));
    logInfo("📡 IP address: " + WiFi.localIP().toString());
  } else if (wm.getConfigPortalActive()) {
    logWarn(String("📶 WiFi config portal active. Connect to '") + AP_NAME + "' to configure WiFi.");
  } else {
    logWarn("⚠️ WiFi not connected and config portal inactive (autoConnect failed).");
  }
}

void processNetwork() {
  auto& wm = getManager();
  wm.process();

  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected && !g_wifiConnected) {
    logInfo("✅ WiFi connection established: " + String(WiFi.SSID()));
    logInfo("📡 IP address: " + WiFi.localIP().toString());
  } else if (!connected && g_wifiConnected) {
    logWarn("⚠️ WiFi connection lost.");
  }
  g_wifiConnected = connected;
}

bool isWiFiConnected() {
  return g_wifiConnected;
}

void resetWiFiSettings() {
  logInfo("🔁 WiFiManager settings are being cleared...");
  auto& wm = getManager();
  wm.resetSettings();
  clockEnabled = false;
  showLeds({});
  delay(EEPROM_WRITE_DELAY_MS);
  ESP.restart();
}
