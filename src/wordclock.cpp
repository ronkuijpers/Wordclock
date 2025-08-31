#include "wordclock.h"
#include "time_mapper.h"
#include "led_state.h"
#include "grid_layout.h"
#include "display_settings.h"



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

  if (!clockEnabled) {
    animating = false;
    showLeds({});
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    logError("❌ Kon tijd niet ophalen");
    return;
  }

  // Determine current rounded bucket and extra minutes
  int minute = timeinfo.tm_min;
  int rounded = (minute / 5) * 5;
  int extra = minute % 5;

  // Start animation when the rounded bucket changes
  if (rounded != lastRounded) {
    lastRounded = rounded;
    segments = get_word_segments_for_time(&timeinfo);
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
    logDebug("🎞️ Start animatie naar nieuwe tekst");
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
    }
    return;
  }

  // Not animating: update full phrase + extra minute LEDs if needed
  std::vector<std::vector<uint16_t>> baseSegs = get_word_segments_for_time(&timeinfo);
  std::vector<uint16_t> indices;
  // Decide whether to include HET/IS based on setting and timer
  uint16_t hisSec = displaySettings.getHetIsDurationSec();
  bool hideHetIs = false;
  if (hisSec == 0) hideHetIs = true;              // never show
  else if (hisSec < 360) {
    hideHetIs = (hetIsVisibleUntil != 0) && (millis() >= hetIsVisibleUntil);
  } // hisSec>=360 => always show

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
