#pragma once
#include <Arduino.h>
#include "config.h"
#include "led_controller.h"
#include "log.h"

class StartupSequence {
public:
  enum State {
    SWEEP,
    DONE
  };

  StartupSequence() : state(SWEEP), index(0), lastUpdate(0) {}

  void start() {
    state = SWEEP;
    index = 0;
    showLeds({});
    lastUpdate = millis();
  logDebug("🔁 Startup: Sweep started");
  }

  void update() {
    unsigned long now = millis();

    switch (state) {
      case SWEEP:
        if (now - lastUpdate >= SWEEP_STEP_MS && index < NUM_LEDS) {
          showLeds({ (uint16_t)index });
          index++;
          lastUpdate = now;
          if (index >= NUM_LEDS) {
            state = DONE;
            showLeds({});
            logInfo("✅ Startup completed");
            return;
          }
        }
        break;

      case DONE:
      default:
        break;
    }
  }

  bool isRunning() const { return state != DONE; }

private:
  State state;
  uint16_t index;
  unsigned long lastUpdate;
};
