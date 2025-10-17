#ifndef NIGHT_MODE_H
#define NIGHT_MODE_H

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

enum class NightModeEffect : uint8_t {
  Off = 0,
  Dim = 1
};

enum class NightModeOverride : uint8_t {
  Auto = 0,
  ForceOn = 1,
  ForceOff = 2
};

class NightMode {
public:
  void begin();

  void updateFromTime(const struct tm& timeinfo);
  void markTimeInvalid();

  bool isEnabled() const { return enabled; }
  void setEnabled(bool on);

  NightModeEffect getEffect() const { return effect; }
  void setEffect(NightModeEffect mode);

  uint8_t getDimPercent() const { return dimPercent; }
  void setDimPercent(uint8_t pct);

  uint16_t getStartMinutes() const { return startMinutes; }
  uint16_t getEndMinutes() const { return endMinutes; }
  void setSchedule(uint16_t startMin, uint16_t endMin);

  NightModeOverride getOverride() const { return overrideMode; }
  void setOverride(NightModeOverride mode);

  bool isActive() const { return active; }
  bool isScheduleActive() const { return scheduleActive; }
  bool hasTime() const { return hasValidTime; }

  uint8_t applyToBrightness(uint8_t baseBrightness) const;

  String formatMinutes(uint16_t minutes) const;
  static bool parseTimeString(const String& text, uint16_t& minutesOut);

private:
  static constexpr const char* PREF_NAMESPACE = "night";

  bool computeScheduleActive(uint16_t minutes) const;
  void persistEnabled();
  void persistEffect();
  void persistDimPercent();
  void persistSchedule();
  void updateEffectiveState(const char* reason);
  void publishState();

  Preferences prefs;
  bool enabled = false;
  NightModeEffect effect = NightModeEffect::Dim;
  uint8_t dimPercent = 20;
  uint16_t startMinutes = 22 * 60;
  uint16_t endMinutes = 6 * 60;
  NightModeOverride overrideMode = NightModeOverride::Auto;
  bool active = false;
  bool scheduleActive = false;
  bool hasValidTime = false;
};

extern NightMode nightMode;

#endif // NIGHT_MODE_H
