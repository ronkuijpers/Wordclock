#pragma once
#include <Preferences.h>
#include <Arduino.h>

class UiAuth {
public:
  void begin(const String &defaultPass) {
    prefs.begin("ui_auth", false);
    bool initialized = prefs.getBool("ui_init", false);
    if (!initialized) {
      // First-time: set defaults and force change
      prefs.putString("ui_user", "user");
      prefs.putString("ui_pass", defaultPass);
      prefs.putBool("mustchg", true);
      prefs.putBool("ui_init", true);
    }
    user = prefs.getString("ui_user", "user");
    pass = prefs.getString("ui_pass", defaultPass);
    mustChange = prefs.getBool("mustchg", true);
    prefs.end();
  }

  const String &getUser() const { return user; }
  const String &getPass() const { return pass; }
  bool needsChange() const { return mustChange; }

  bool setPassword(const String &newPass) {
    if (newPass.length() < 6) return false;
    pass = newPass;
    mustChange = false;
    prefs.begin("ui_auth", false);
    prefs.putString("ui_pass", pass);
    prefs.putBool("mustchg", mustChange);
    prefs.end();
    return true;
  }

private:
  Preferences prefs;
  String user;
  String pass;
  bool mustChange = true;
};

extern UiAuth uiAuth;

