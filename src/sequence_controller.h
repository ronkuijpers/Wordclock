#pragma once
#include <Arduino.h>
#include <vector>

#include "config.h"
#include "grid_layout.h"
#include "led_controller.h"
#include "log.h"

class StartupSequence {
public:
  enum State {
    SWEEP,
    WORD_WALK,
    WORD_HOLD,
    DONE
  };

  void setWordWalkEnabled(bool enabled) { wordWalkEnabled = enabled; }

  StartupSequence()
    : state(SWEEP),
      sweepIndex(0),
      wordIndex(0),
      lastUpdate(0),
      wordWalkEnabled(true) {}

  void start() {
    state = SWEEP;
    sweepIndex = 0;
    wordIndex = 0;
    showLeds({});
    lastUpdate = millis();
    logDebug("🔁 Startup: Sweep started");
  }

  void update() {
    unsigned long now = millis();

    switch (state) {
      case SWEEP: {
        const uint16_t totalLeds = getActiveLedCountTotal();
        if (now - lastUpdate >= SWEEP_STEP_MS && sweepIndex < totalLeds) {
          showLeds({ static_cast<uint16_t>(sweepIndex) });
          sweepIndex++;
          lastUpdate = now;
          if (sweepIndex >= totalLeds) {
            logDebug(wordWalkEnabled ? "🔁 Startup: Sweep finished, starting word walk"
                                     : "🔁 Startup: Sweep finished, skipping word walk");
            transitionToWordWalk(now);
          }
        } else if (sweepIndex >= totalLeds) {
          logDebug(wordWalkEnabled ? "🔁 Startup: Sweep finished (adjusted), starting word walk"
                                   : "🔁 Startup: Sweep finished (adjusted), skipping word walk");
          transitionToWordWalk(now);
        }
        break;
      }

      case WORD_WALK:
        if (wordIndex >= ACTIVE_WORD_COUNT || !ACTIVE_WORDS) {
          transitionToHold(now);
          break;
        }
        if ((now - lastUpdate) >= WORD_SEQUENCE_STEP_MS) {
          displayWord(wordIndex);
          wordIndex++;
          lastUpdate = now;
          if (wordIndex >= ACTIVE_WORD_COUNT) {
            transitionToHold(now);
          }
        }
        break;

      case WORD_HOLD:
        if (now - lastUpdate >= WORD_SEQUENCE_HOLD_MS) {
          showLeds({});
          state = DONE;
          logInfo("✅ Startup completed");
        }
        break;

      case DONE:
      default:
        break;
    }
  }

  bool isRunning() const { return state != DONE; }

private:
  void transitionToWordWalk(unsigned long now) {
    if (!wordWalkEnabled) {
      state = DONE;
      showLeds({});
      lastUpdate = now;
      logInfo("✅ Startup completed");
      return;
    }
    state = WORD_WALK;
    wordIndex = 0;
    lastUpdate = now ? now : millis();
    buffer.clear();
    showLeds({});
    if (ACTIVE_WORD_COUNT == 0 || !ACTIVE_WORDS) {
      transitionToHold(lastUpdate);
    } else {
      // Force immediate first word by backdating timer
      lastUpdate = lastUpdate > WORD_SEQUENCE_STEP_MS ? lastUpdate - WORD_SEQUENCE_STEP_MS : 0;
    }
  }

  void transitionToHold(unsigned long now) {
    state = WORD_HOLD;
    lastUpdate = now;
  }

  void displayWord(size_t idx) {
    if (!ACTIVE_WORDS || idx >= ACTIVE_WORD_COUNT) return;
    buffer.clear();
    const WordPosition& wp = ACTIVE_WORDS[idx];
    for (int i = 0; i < 20 && wp.indices[i] != 0; ++i) {
      if (wp.indices[i] < 0) continue;
      buffer.push_back(static_cast<uint16_t>(wp.indices[i]));
    }
    if (buffer.empty()) {
      showLeds({});
    } else {
      showLeds(buffer);
    }
  }

  State state;
  uint16_t sweepIndex;
  size_t wordIndex;
  unsigned long lastUpdate;
  std::vector<uint16_t> buffer;
  bool wordWalkEnabled;
};
