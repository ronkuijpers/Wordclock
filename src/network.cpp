#include <WiFiManager.h>
#include "log.h"

void resetWiFiSettings() {
  logInfo("🔁 WiFiManager settings are being cleared...");
  WiFiManager wm;
  wm.resetSettings();     // <-- important
  delay(EEPROM_WRITE_DELAY_MS); // give the EEPROM some time
  ESP.restart();
}
