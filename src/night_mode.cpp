#include "night_mode.h"
#include "log.h"
#include "mqtt_client.h"

NightMode nightMode;

void NightMode::begin() {
  prefs.begin(PREF_NAMESPACE, false);
  enabled = prefs.getBool("enabled", false);
  uint8_t storedEffect = prefs.getUChar("effect", static_cast<uint8_t>(NightModeEffect::Dim));
  if (storedEffect > static_cast<uint8_t>(NightModeEffect::Dim)) {
    storedEffect = static_cast<uint8_t>(NightModeEffect::Dim);
  }
  effect = static_cast<NightModeEffect>(storedEffect);
  dimPercent = prefs.getUChar("dim_pct", 20);
  if (dimPercent > 100) dimPercent = 100;
  startMinutes = prefs.getUShort("start", 22 * 60);
  if (startMinutes >= 24 * 60) startMinutes = 22 * 60;
  endMinutes = prefs.getUShort("end", 6 * 60);
  if (endMinutes >= 24 * 60) endMinutes = 6 * 60;
  prefs.end();

  overrideMode = NightModeOverride::Auto;
  active = false;
  scheduleActive = false;
  hasValidTime = false;
}

void NightMode::updateFromTime(const struct tm& timeinfo) {
  uint16_t minutes = static_cast<uint16_t>((timeinfo.tm_hour * 60) + timeinfo.tm_min);
  bool newScheduleActive = computeScheduleActive(minutes);
  bool scheduleChanged = (newScheduleActive != scheduleActive);
  scheduleActive = newScheduleActive;
  hasValidTime = true;
  if (scheduleChanged) {
    updateEffectiveState("schedule");
  } else if (overrideMode == NightModeOverride::Auto) {
    // In auto mode we still need to keep active state aligned with schedule/enabled/time
    updateEffectiveState(nullptr);
  }
}

void NightMode::markTimeInvalid() {
  if (!hasValidTime) return;
  hasValidTime = false;
  if (overrideMode == NightModeOverride::Auto && active) {
    updateEffectiveState("time-invalid");
  }
}

void NightMode::setEnabled(bool on) {
  if (enabled == on) return;
  enabled = on;
  persistEnabled();
  logInfo(String("🌙 Night mode ") + (enabled ? "enabled" : "disabled"));
  updateEffectiveState("enabled");
}

void NightMode::setEffect(NightModeEffect mode) {
  if (effect == mode) return;
  effect = mode;
  persistEffect();
  const char* label = (effect == NightModeEffect::Off) ? "off" : "dim";
  logInfo(String("🌙 Night mode effect -> ") + label);
  updateEffectiveState("effect");
}

void NightMode::setDimPercent(uint8_t pct) {
  if (pct > 100) pct = 100;
  if (dimPercent == pct) return;
  dimPercent = pct;
  persistDimPercent();
  logInfo(String("🌙 Night mode dim -> ") + pct + "%");
  publishState();
}

void NightMode::setSchedule(uint16_t startMin, uint16_t endMin) {
  startMin %= (24 * 60);
  endMin %= (24 * 60);
  if (startMinutes == startMin && endMinutes == endMin) return;
  startMinutes = startMin;
  endMinutes = endMin;
  persistSchedule();
  logInfo(String("🌙 Night schedule -> ") + formatMinutes(startMinutes) + " - " + formatMinutes(endMinutes));
  if (overrideMode == NightModeOverride::Auto && hasValidTime) {
    updateEffectiveState("schedule-update");
  }
}

void NightMode::setOverride(NightModeOverride mode) {
  if (overrideMode == mode) return;
  overrideMode = mode;
  const char* label = "auto";
  if (mode == NightModeOverride::ForceOn) label = "force-on";
  else if (mode == NightModeOverride::ForceOff) label = "force-off";
  logInfo(String("🌙 Night override -> ") + label);
  updateEffectiveState("override");
}

uint8_t NightMode::applyToBrightness(uint8_t baseBrightness) const {
  if (!active) return baseBrightness;
  if (effect == NightModeEffect::Off) {
    return 0;
  }
  uint16_t scaled = static_cast<uint16_t>(baseBrightness) * dimPercent;
  uint8_t result = static_cast<uint8_t>(scaled / 100);
  if (dimPercent > 0 && baseBrightness > 0 && result == 0) {
    result = 1; // avoid rounding to zero when dimming but base is small
  }
  return result;
}

String NightMode::formatMinutes(uint16_t minutes) const {
  minutes %= (24 * 60);
  uint8_t hour = minutes / 60;
  uint8_t minute = minutes % 60;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", hour, minute);
  return String(buf);
}

bool NightMode::parseTimeString(const String& text, uint16_t& minutesOut) {
  String trimmed = text;
  trimmed.trim();
  if (trimmed.length() < 4) return false;
  int colon = trimmed.indexOf(':');
  if (colon <= 0) return false;
  String hStr = trimmed.substring(0, colon);
  String mStr = trimmed.substring(colon + 1);
  if (hStr.length() == 0 || mStr.length() == 0) return false;
  int hour = hStr.toInt();
  int minute = mStr.toInt();
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return false;
  minutesOut = static_cast<uint16_t>(hour * 60 + minute);
  return true;
}

bool NightMode::computeScheduleActive(uint16_t minutes) const {
  if (!enabled) return false;
  if (startMinutes == endMinutes) {
    return false; // zero-length window
  }
  if (startMinutes < endMinutes) {
    return minutes >= startMinutes && minutes < endMinutes;
  }
  // Wrap around midnight
  return minutes >= startMinutes || minutes < endMinutes;
}

void NightMode::persistEnabled() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putBool("enabled", enabled);
  prefs.end();
}

void NightMode::persistEffect() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putUChar("effect", static_cast<uint8_t>(effect));
  prefs.end();
}

void NightMode::persistDimPercent() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putUChar("dim_pct", dimPercent);
  prefs.end();
}

void NightMode::persistSchedule() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putUShort("start", startMinutes);
  prefs.putUShort("end", endMinutes);
  prefs.end();
}

void NightMode::updateEffectiveState(const char* reason) {
  bool newActive;
  if (overrideMode == NightModeOverride::ForceOn) {
    newActive = true;
  } else if (overrideMode == NightModeOverride::ForceOff) {
    newActive = false;
  } else {
    newActive = (enabled && hasValidTime && scheduleActive);
  }
  if (newActive == active) {
    if (reason == nullptr) {
      return;
    }
    // Still publish to keep MQTT consumers updated for dim percent change etc.
    publishState();
    return;
  }
  active = newActive;
  const char* label = reason ? reason : "state-change";
  logInfo(String("🌙 Night mode ") + (active ? "ACTIVE" : "INACTIVE") + " (" + label + ")");
  publishState();
}

void NightMode::publishState() {
  mqtt_publish_state(true);
}
