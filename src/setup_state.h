#pragma once

#include <Preferences.h>

// Tracks whether the initial setup wizard has been completed.
class SetupState {
public:
  void begin(bool hasLegacyConfig);
  void markComplete();
  void reset();

  bool isComplete() const { return completed; }
  uint8_t getVersion() const { return version; }
  bool wasMigrated() const { return migratedFromLegacy; }

private:
  void persist();

  Preferences prefs;
  bool completed = false;
  uint8_t version = 0;
  bool migratedFromLegacy = false;
};

extern SetupState setupState;
