#pragma once
#include <Preferences.h>

class DisplaySettings {
public:
  void begin() {
    prefs.begin("display", false);
    hetIsDurationSec = prefs.getUShort("his_sec", 30); // default 30s
    if (hetIsDurationSec > 360) hetIsDurationSec = 360;
    prefs.end();
  }

  uint16_t getHetIsDurationSec() const { return hetIsDurationSec; }

  void setHetIsDurationSec(uint16_t s) {
    if (s > 360) s = 360;
    hetIsDurationSec = s;
    prefs.begin("display", false);
    prefs.putUShort("his_sec", hetIsDurationSec);
    prefs.end();
  }

private:
  uint16_t hetIsDurationSec = 30;
  Preferences prefs;
};

extern DisplaySettings displaySettings;

