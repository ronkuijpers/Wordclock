#pragma once
#include <Preferences.h>

class DisplaySettings {
public:
  void begin() {
    prefs.begin("display", false);
    hetIsDurationSec = prefs.getUShort("his_sec", 30); // default 30s
    if (hetIsDurationSec > 360) hetIsDurationSec = 360;
    sellMode = prefs.getBool("sell_on", false);
    prefs.end();
  }

  uint16_t getHetIsDurationSec() const { return hetIsDurationSec; }
  bool isSellMode() const { return sellMode; }

  void setHetIsDurationSec(uint16_t s) {
    if (s > 360) s = 360;
    hetIsDurationSec = s;
    prefs.begin("display", false);
    prefs.putUShort("his_sec", hetIsDurationSec);
    prefs.end();
  }
  void setSellMode(bool on) {
    sellMode = on;
    prefs.begin("display", false);
    prefs.putBool("sell_on", sellMode);
    prefs.end();
  }

private:
  uint16_t hetIsDurationSec = 30;
  bool sellMode = false;
  Preferences prefs;
};

extern DisplaySettings displaySettings;
