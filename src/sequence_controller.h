#pragma once
#include <stdint.h>
#include <vector>
#include "led_controller.h"
#include "grid_layout.h"


class StartupSequence {
public:
  enum State { INACTIVE, FORWARD, PAUSE, BACKWARD, COMPLETE };

  StartupSequence() : state(INACTIVE), index(0), lastUpdate(0) {}

  void start() {
    state = FORWARD;
    index = 0;
    lastUpdate = millis();
  }

  void update() {
    unsigned long now = millis();
    switch (state) {
      case FORWARD:
        if (now - lastUpdate >= 50 && index < LED_COUNT_TOTAL) {
          showLeds({index});
          index++;
          lastUpdate = now;
          if (index >= LED_COUNT_TOTAL) {
            state = PAUSE;
            lastUpdate = now;
          }
        }
        break;

      case PAUSE:
        if (now - lastUpdate >= 500) {
          state = BACKWARD;
          index = 0;
          lastUpdate = now;
        }
        break;

      case BACKWARD:
        if (now - lastUpdate >= 50 && index < LED_COUNT_TOTAL) {
          showLeds({});
          index++;
          lastUpdate = now;
          if (index >= LED_COUNT_TOTAL) {
            state = COMPLETE;
          }
        }
        break;

      default:
        break;
    }
  }

  bool isRunning() const { return state != INACTIVE && state != COMPLETE; }
  bool isDone() const { return state == COMPLETE; }

private:
  State state;
  uint16_t index;
  unsigned long lastUpdate;
};
