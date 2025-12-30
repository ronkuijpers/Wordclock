#include "wordclock.h"
#include "time_mapper.h"
#include "led_state.h"
#include "grid_layout.h"
#include "display_settings.h"
#include "time_sync.h"
#include "night_mode.h"
#include "setup_state.h"
#include <algorithm>
#include <cstring>



static bool g_forceAnim = false;
static struct tm g_forcedTime = {};
static unsigned long g_noTimeIndicatorStart = 0;
static std::vector<uint16_t> g_noTimeIndicatorLeds;
static bool g_loggedInitialTimeFailure = false;
static std::vector<WordSegment> g_lastSegments;
static std::vector<WordSegment> g_animTargetSegments;
static std::vector<std::vector<uint16_t>> g_animFrames;

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

static bool isHetIs(const WordSegment& seg) {
  return strcmp(seg.key, "HET") == 0 || strcmp(seg.key, "IS") == 0;
}

static void stripHetIsIfDisabled(std::vector<WordSegment>& segs, uint16_t hetIsDurationSec) {
  if (hetIsDurationSec != 0) return;
  segs.erase(std::remove_if(segs.begin(), segs.end(), [](const WordSegment& s) { return isHetIs(s); }), segs.end());
}

static std::vector<uint16_t> flattenSegments(const std::vector<WordSegment>& segs) {
  std::vector<uint16_t> indices;
  for (const auto& seg : segs) {
    indices.insert(indices.end(), seg.leds.begin(), seg.leds.end());
  }
  return indices;
}

static const WordSegment* findSegment(const std::vector<WordSegment>& segs, const char* key) {
  for (const auto& seg : segs) {
    if (strcmp(seg.key, key) == 0) return &seg;
  }
  return nullptr;
}

static void removeLeds(std::vector<uint16_t>& base, const std::vector<uint16_t>& toRemove) {
  base.erase(std::remove_if(base.begin(), base.end(), [&](uint16_t idx) {
    return std::find(toRemove.begin(), toRemove.end(), idx) != toRemove.end();
  }), base.end());
}

static bool hetIsCurrentlyVisible(uint16_t hetIsDurationSec, unsigned long hetIsVisibleUntil, unsigned long nowMs) {
  if (hetIsDurationSec == 0) return false;
  if (hetIsDurationSec >= 360) return true;
  if (hetIsVisibleUntil == 0) return true;
  return nowMs < hetIsVisibleUntil;
}

static void buildClassicFrames(const std::vector<WordSegment>& segs, std::vector<std::vector<uint16_t>>& frames) {
  frames.clear();
  std::vector<uint16_t> cumulative;
  for (const auto& seg : segs) {
    cumulative.insert(cumulative.end(), seg.leds.begin(), seg.leds.end());
    frames.push_back(cumulative);
  }
}

static void buildSmartFrames(const std::vector<WordSegment>& prevSegments,
                             const std::vector<WordSegment>& nextSegments,
                             bool hetIsVisible,
                             std::vector<std::vector<uint16_t>>& frames) {
  frames.clear();
  if (prevSegments.empty()) {
    buildClassicFrames(nextSegments, frames);
    return;
  }

  std::vector<WordSegment> prevVisible;
  prevVisible.reserve(prevSegments.size());
  for (const auto& seg : prevSegments) {
    if (isHetIs(seg) && !hetIsVisible) continue;
    prevVisible.push_back(seg);
  }

  std::vector<uint16_t> current = flattenSegments(prevVisible);

  std::vector<WordSegment> removals;
  for (const auto& seg : prevVisible) {
    bool presentInNext = findSegment(nextSegments, seg.key) != nullptr;
    if (isHetIs(seg) || !presentInNext) {
      removals.push_back(seg);
    }
  }

  std::vector<WordSegment> additions;
  for (const auto& seg : nextSegments) {
    bool visibleBefore = findSegment(prevVisible, seg.key) != nullptr;
    if (isHetIs(seg) || !visibleBefore) {
      additions.push_back(seg);
    }
  }

  if (!removals.empty()) {
    std::vector<uint16_t> removalLeds;
    for (const auto& rem : removals) {
      removalLeds.insert(removalLeds.end(), rem.leds.begin(), rem.leds.end());
    }
    removeLeds(current, removalLeds);
    frames.push_back(current);
  }
  for (const auto& add : additions) {
    current.insert(current.end(), add.leds.begin(), add.leds.end());
    frames.push_back(current);
  }
  if (frames.empty()) {
    frames.push_back(current);
  }
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
    g_animTargetSegments = get_word_segments_with_keys(&animTime);
    g_animFrames.clear();
    uint16_t hisSec = displaySettings.getHetIsDurationSec();
    stripHetIsIfDisabled(g_animTargetSegments, hisSec);
    bool animate = displaySettings.getAnimateWords();
    WordAnimationMode mode = displaySettings.getAnimationMode();

    if (animate) {
      bool hetIsVisible = hetIsCurrentlyVisible(hisSec, hetIsVisibleUntil, nowMs);
      if (mode == WordAnimationMode::Smart && !g_lastSegments.empty()) {
        buildSmartFrames(g_lastSegments, g_animTargetSegments, hetIsVisible, g_animFrames);
      } else {
        buildClassicFrames(g_animTargetSegments, g_animFrames);
        mode = WordAnimationMode::Classic; // normalize for logging
      }
      if (g_animFrames.empty()) {
        animating = false;
      } else {
        animStep = 0;
        lastStepAt = millis();
        animating = true;
        hetIsVisibleUntil = 0; // reset; will be set when animation completes
        String m = (mode == WordAnimationMode::Smart) ? "smart" : "classic";
        logDebug(String("🎞️ Start animation to new text (") + m + ")");
      }
    } else {
      animating = false;
    }

    if (!animating) {
      // Animation skipped or no steps -> immediately consider the animation 'completed'
      if (hisSec >= 360) {
        hetIsVisibleUntil = 0; // always on
      } else if (hisSec == 0) {
        hetIsVisibleUntil = 1; // hidden immediately
      } else {
        hetIsVisibleUntil = millis() + (unsigned long)hisSec * 1000UL;
      }
      g_lastSegments = g_animTargetSegments;
    }
    g_forceAnim = false;
  }

  // During animation: add next frame every 500ms
  if (animating) {
    unsigned long now = millis();
    unsigned long deltaMs = (animStep == 0) ? 0 : (now - lastStepAt);
    if (animStep == 0 || deltaMs >= 500) {
      if (animStep < (int)g_animFrames.size()) {
        const auto& frame = g_animFrames[animStep];
        size_t prevSize = (animStep == 0) ? 0 : g_animFrames[animStep - 1].size();
        int stepIndex = animStep; // capture current step (0-based) before increment
        animStep++;
        String msg = "Anim step ";
        msg += (stepIndex + 1);
        msg += "/";
        msg += g_animFrames.size();
        msg += " dt=";
        msg += deltaMs;
        msg += "ms (Δ";
        msg += (int)frame.size() - (int)prevSize;
        msg += " leds)";
        if (deltaMs > 700) {
          msg += " ⚠️ slow";
          logWarn(msg);
        } else {
          logDebug(msg);
        }
        showLeds(frame);
        lastStepAt = now;
      } else {
        animating = false;
      }
    } else if (animStep > 0 && animStep <= (int)g_animFrames.size()) {
      showLeds(g_animFrames[animStep - 1]);
    }

    if (animStep >= (int)g_animFrames.size()) {
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
      g_lastSegments = g_animTargetSegments;
  logDebug(String("Animation completed; steps=") + g_animFrames.size() + String(", HET IS duration=") + (hisSec>=360?"always": (hisSec==0?"off":String(hisSec)+"s")));
    }
    return;
  }

  // Not animating: update full phrase + extra minute LEDs if needed
  std::vector<WordSegment> baseSegs = get_word_segments_with_keys(&effective);
  uint16_t hisSec = displaySettings.getHetIsDurationSec();
  stripHetIsIfDisabled(baseSegs, hisSec);

  std::vector<uint16_t> indices;
  // Decide whether to include HET/IS based on setting and timer
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

  for (const auto& seg : baseSegs) {
    if (hideHetIs && isHetIs(seg)) continue;
    indices.insert(indices.end(), seg.leds.begin(), seg.leds.end());
  }
  for (int i = 0; i < extra && i < 4; ++i) {
    indices.push_back(EXTRA_MINUTE_LEDS[i]);
  }
  showLeds(indices);
  g_lastSegments = baseSegs;
}

void wordclock_force_animation_for_time(struct tm* timeinfo) {
  if (!timeinfo) return;
  g_forcedTime = *timeinfo;
  g_forceAnim = true;
}
