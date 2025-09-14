#pragma once
#include <Preferences.h>

class DisplaySettings {
public:
  void begin() {
    prefs.begin("display", false);
    hetIsDurationSec = prefs.getUShort("his_sec", 360); // default ALWAYS (360s)
    if (hetIsDurationSec > 360) hetIsDurationSec = 360;
    sellMode = prefs.getBool("sell_on", false);
    animateWords = prefs.getBool("anim_on", false); // default OFF unless enabled via UI
    autoUpdate = prefs.getBool("auto_upd", true);   // default ON to keep current behavior
    prefs.end();
  }

  uint16_t getHetIsDurationSec() const { return hetIsDurationSec; }
  bool isSellMode() const { return sellMode; }
  bool getAnimateWords() const { return animateWords; }
  bool getAutoUpdate() const { return autoUpdate; }

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

  void setAnimateWords(bool on) {
    animateWords = on;
    prefs.begin("display", false);
    prefs.putBool("anim_on", animateWords);
    prefs.end();
  }

  void setAutoUpdate(bool on) {
    autoUpdate = on;
    prefs.begin("display", false);
    prefs.putBool("auto_upd", autoUpdate);
    prefs.end();
  }

private:
  uint16_t hetIsDurationSec = 360; // default ALWAYS
  bool sellMode = false;
  bool animateWords = false; // default OFF
  bool autoUpdate = true;    // default ON
  Preferences prefs;
};

extern DisplaySettings displaySettings;
