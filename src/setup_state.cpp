#include "setup_state.h"

#include "log.h"

namespace {
constexpr uint8_t SETUP_STATE_VERSION = 1;
constexpr const char* PREF_NAMESPACE = "setup";
}

SetupState setupState;

void SetupState::persist() {
  prefs.putBool("done", completed);
  prefs.putUChar("ver", version);
}

void SetupState::begin(bool hasLegacyConfig) {
  prefs.begin(PREF_NAMESPACE, false);
  migratedFromLegacy = false;
  const bool hasDoneKey = prefs.isKey("done");
  const bool hasVerKey = prefs.isKey("ver");
  completed = prefs.getBool("done", false);
  version = prefs.getUChar("ver", 0);

  const bool firstInit = !hasDoneKey && !hasVerKey;
  if (firstInit) {
    // For devices that already had config (WiFi/grid), assume setup was done.
    if (hasLegacyConfig) {
      completed = true;
      migratedFromLegacy = true;
      logInfo("ℹ️ Setup state initialized as completed (legacy config detected)");
    } else {
      completed = false;
      logInfo("ℹ️ Setup state initialized as pending");
    }
  }

  // Always bump to current schema version
  version = SETUP_STATE_VERSION;
  persist();
  prefs.end();
}

void SetupState::markComplete() {
  completed = true;
  prefs.begin(PREF_NAMESPACE, false);
  version = SETUP_STATE_VERSION;
  persist();
  prefs.end();
  logInfo("✅ Setup marked as complete");
}

void SetupState::reset() {
  completed = false;
  prefs.begin(PREF_NAMESPACE, false);
  version = SETUP_STATE_VERSION;
  persist();
  prefs.end();
  logInfo("ℹ️ Setup state reset (wizard required)");
}
