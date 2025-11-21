#include "wordclock.h"
#include "time_mapper.h"
#include "led_state.h"
#include "grid_layout.h"
#include "display_settings.h"
#include "time_sync.h"
#include "night_mode.h"
#include "setup_state.h"



static bool g_forceAnim = false;
static struct tm g_forcedTime = {};
static unsigned long g_noTimeIndicatorStart = 0;
static std::vector<uint16_t> g_noTimeIndicatorLeds;
static bool g_loggedInitialTimeFailure = false;

static void ensureNoTimeIndicatorLeds() {
  if (!g_noTimeIndicatorLeds.empty()) return;
  size_t count = EXTRA_MINUTE_LED_COUNT >= 4 ? 4 : EXTRA_MINUTE_LED_COUNT;
  for (size_t i = 0; i < count; ++i) {
    g_noTimeIndicatorLeds.push_back(EXTRA_MINUTE_LEDS[i]);
  }
}

static void showNoTimeIndicator(unsigned long nowMs) {
  ensureNoTimeIndicatorLeds();
  if (g_noTimeIndicatorStart == 0) {
    g_noTimeIndicatorStart = nowMs;
  }
  const unsigned long elapsed = nowMs - g_noTimeIndicatorStart;
  const unsigned long phase = elapsed % 5000UL; // 5 second cycle
  if (phase < 500UL) {
    showLeds(g_noTimeIndicatorLeds);
  } else {
    showLeds({});
  }
}

static void resetNoTimeIndicator() {
  g_noTimeIndicatorStart = 0;
  g_noTimeIndicatorLeds.clear();
}

void wordclock_setup() {
  // ledState.begin() is initialized in main
  initLeds();
  logInfo("Wordclock setup complete");
}

void wordclock_loop() {
  static bool animating = false;
  static unsigned long lastStepAt = 0;
  static int animStep = 0;
  static int lastRounded = -1;
  static std::vector<std::vector<uint16_t>> segments;
  static std::vector<uint16_t> cumulative;
  static unsigned long hetIsVisibleUntil = 0; // millis timestamp when HET+IS should turn off
  // Cache time to avoid calling getLocalTime() every 50ms
  static struct tm cachedTime = {};
  static bool haveTime = false;
  static unsigned long lastTimeFetchMs = 0;

  unsigned long nowMs = millis();

  if (!clockEnabled) {
    animating = false;
    showLeds({});
    resetNoTimeIndicator();
    return;
  }

  if (!setupState.isComplete()) {
    animating = false;
    showLeds({});
    resetNoTimeIndicator();
    return;
  }

  // Refresh cached time at most once per second
  if (!haveTime || (nowMs - lastTimeFetchMs) >= 1000UL) {
    struct tm t = {};
    if (getLocalTime(&t)) {
      cachedTime = t;
      haveTime = true;
      lastTimeFetchMs = nowMs;
      g_initialTimeSyncSucceeded = true;
      g_loggedInitialTimeFailure = false;
      nightMode.updateFromTime(cachedTime);
      resetNoTimeIndicator();
    } else if (!haveTime) {
      if (!g_loggedInitialTimeFailure) {
        logWarn("❗ Unable to fetch time; showing no-time indicator");
        g_loggedInitialTimeFailure = true;
      }
      nightMode.markTimeInvalid();
      showNoTimeIndicator(nowMs);
      return;
    }
  }
  if (!haveTime) {
    nightMode.markTimeInvalid();
    showNoTimeIndicator(nowMs);
    return;
  }
  struct tm timeinfo = cachedTime;

  // Debug: log every minute change
  static int lastLoggedMinute = -1;
  if (timeinfo.tm_min != lastLoggedMinute) {
    lastLoggedMinute = timeinfo.tm_min;
    char buf[20];
    snprintf(buf, sizeof(buf), "🔄 %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    logDebug(String(buf));
  }

  // Determine current rounded bucket and extra minutes
  // Apply sell-mode override (forces 10:47)
  struct tm effective = timeinfo;
  if (displaySettings.isSellMode()) {
    effective.tm_hour = 10;
    effective.tm_min = 47;
  }

  int minute = effective.tm_min;
  int rounded = (minute / 5) * 5;
  int extra = minute % 5;

  // Start animation when the rounded bucket changes or when forced externally
  if (rounded != lastRounded || g_forceAnim) {
    lastRounded = rounded;
    struct tm animTime = effective;
    if (g_forceAnim) {
      animTime = g_forcedTime;
    }
    if (displaySettings.getAnimateWords()) {
      segments = get_word_segments_for_time(&animTime);
      // Respect setting: if duration==0, skip HET/IS entirely (both animation and steady state)
      uint16_t hisSec = displaySettings.getHetIsDurationSec();
      if (hisSec == 0 && segments.size() >= 2) {
        segments.erase(segments.begin(), segments.begin() + 2); // drop HET and IS
      }
      cumulative.clear();
      animStep = 0;
      lastStepAt = millis();
      animating = true;
      hetIsVisibleUntil = 0; // reset; will be set when animation completes
  logDebug("🎞️ Start animation to new text");
      g_forceAnim = false;
    } else {
      // No animation: immediately consider the animation 'completed' for timer purposes
      animating = false;
      uint16_t hisSec = displaySettings.getHetIsDurationSec();
      if (hisSec >= 360) {
        hetIsVisibleUntil = 0; // always on
      } else if (hisSec == 0) {
        hetIsVisibleUntil = 1; // hidden immediately
      } else {
        hetIsVisibleUntil = millis() + (unsigned long)hisSec * 1000UL;
      }
      g_forceAnim = false;
    }
  }

  // During animation: add next word every 500ms
  if (animating) {
    unsigned long now = millis();
    if (animStep == 0 || now - lastStepAt >= 500) {
      if (animStep < (int)segments.size()) {
        // Append this segment
        const auto &seg = segments[animStep];
        cumulative.insert(cumulative.end(), seg.begin(), seg.end());
        animStep++;
        lastStepAt = now;
      }
    }
    // Show accumulated words (no extra minutes yet at a 5-min boundary)
    showLeds(cumulative);
    if (animStep >= (int)segments.size()) {
      animating = false;
      // Start timer for hiding HET+IS now that full text is shown
      uint16_t hisSec = displaySettings.getHetIsDurationSec();
      if (hisSec >= 360) {
        hetIsVisibleUntil = 0; // always on
      } else if (hisSec == 0) {
        hetIsVisibleUntil = 1; // already expired -> hide immediately
      } else {
        hetIsVisibleUntil = millis() + (unsigned long)hisSec * 1000UL;
      }
  logDebug(String("Animation completed; segments=") + segments.size() + String(", HET IS duration=") + (hisSec>=360?"always": (hisSec==0?"off":String(hisSec)+"s")));
    }
    return;
  }

  // Not animating: update full phrase + extra minute LEDs if needed
  std::vector<std::vector<uint16_t>> baseSegs = get_word_segments_for_time(&effective);
  std::vector<uint16_t> indices;
  // Decide whether to include HET/IS based on setting and timer
  uint16_t hisSec = displaySettings.getHetIsDurationSec();
  static bool lastHetIsHidden = false;
  bool hideHetIs = false;
  if (hisSec == 0) hideHetIs = true;              // never show
  else if (hisSec < 360) {
    hideHetIs = (hetIsVisibleUntil != 0) && (millis() >= hetIsVisibleUntil);
  } // hisSec>=360 => always show

  if (hideHetIs && !lastHetIsHidden) {
  logDebug("'HET IS' hidden after configured duration");
  }
  lastHetIsHidden = hideHetIs;

  for (size_t si = 0; si < baseSegs.size(); ++si) {
    // baseSegs[0] == HET, baseSegs[1] == IS per mapper design
    if (hideHetIs && (si == 0 || si == 1)) continue;
    const auto &seg = baseSegs[si];
    indices.insert(indices.end(), seg.begin(), seg.end());
  }
  for (int i = 0; i < extra && i < 4; ++i) {
    indices.push_back(EXTRA_MINUTE_LEDS[i]);
  }
  showLeds(indices);
}

void wordclock_force_animation_for_time(struct tm* timeinfo) {
  if (!timeinfo) return;
  g_forcedTime = *timeinfo;
  g_forceAnim = true;
}
