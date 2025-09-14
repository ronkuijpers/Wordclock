#pragma once
#include <vector>
#include <string>
#include <WiFi.h>
#include "led_controller.h"
#include "grid_layout.h"
#include "time_mapper.h"
#include "log.h"

class StartupSequence {
public:
  enum State {
    SWEEP,
    IP_DIGITS,
    IP_LAST_HOLD,
    IP_PAUSE,
    DONE
  };

  StartupSequence() : state(SWEEP), index(0), lastUpdate(0), step(0) {}

  void start() {
    state = SWEEP;
    index = 0;
    step = 0;
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
            state = IP_DIGITS;
            logDebug("📡 Startup: IP address animation");
            prepareIPSequence();
            // Toon direct het eerste element van het IP-adres
            if (!ipSequence.empty()) {
              showLeds(ipSequence[0]);
              logDebug(String("🔢 IP part shown: ") + ipLabels[0]);
              step = 1;
            } else {
              step = 0;
            }
            lastUpdate = now;
          }
        }
        break;

      case IP_DIGITS:
        if (now - lastUpdate >= IP_STEP_MS && step < ipSequence.size()) {
          showLeds(ipSequence[step]);
          logDebug(String("🔢 IP part shown: ") + ipLabels[step]);
          step++;
          lastUpdate = now;
          if (step >= ipSequence.size()) {
            // Laat het laatste IP-deel nog IP_STEP_MS zichtbaar blijven
            state = IP_LAST_HOLD;
            lastUpdate = now;
          }
        }
        break;

      case IP_LAST_HOLD:
        // Short intermediate step: keep last shown digit/dot visible,
        // then clear before entering pause.
        if (now - lastUpdate >= IP_STEP_MS) {
          showLeds({});
          state = IP_PAUSE;
          lastUpdate = now;
          logDebug("⏳ Startup: pause after IP display");
        }
        break;

      case IP_PAUSE:
        if (now - lastUpdate >= 2000) {
          state = DONE;
          showLeds({});
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
  State state;
  uint16_t index;
  unsigned long lastUpdate;
  int step;
  std::vector<std::vector<uint16_t>> ipSequence;
  std::vector<String> ipLabels;
  // Timing and LED indices from config.h are now used directly as global constexprs from config.h

  void prepareIPSequence() {
    IPAddress ip = WiFi.localIP();
    ipSequence.clear();
    ipLabels.clear();

    for (int i = 0; i < 4; ++i) {
      int part = ip[i];
      String partStr = String(part);
      for (int j = 0; j < partStr.length(); ++j) {
        char digit = partStr[j];
        std::string word;
        switch (digit) {
          case '0': {
            // Toon '0' als de letter 'O' uit de grid (links in "OVER")
            ipSequence.push_back({ static_cast<uint16_t>(IP_ZERO_O_LED_INDEX) });
            ipLabels.push_back("0");
            continue;
          }
          case '1': word = "EEN"; break;
          case '2': word = "TWEE"; break;
          case '3': word = "DRIE"; break;
          case '4': word = "VIER"; break;
          case '5': word = "VIJF"; break;
          case '6': word = "ZES"; break;
          case '7': word = "ZEVEN"; break;
          case '8': word = "ACHT"; break;
          case '9': word = "NEGEN"; break;
        }
        ipSequence.push_back(get_leds_for_word(word.c_str()));
        ipLabels.push_back(String(digit));
      }
      if (i < 3) {
        // Scheidingsteken '.': toon alle 4 minuten-LEDs
        ipSequence.push_back({
          static_cast<uint16_t>(EXTRA_MINUTE_LEDS[0]),
          static_cast<uint16_t>(EXTRA_MINUTE_LEDS[1]),
          static_cast<uint16_t>(EXTRA_MINUTE_LEDS[2]),
          static_cast<uint16_t>(EXTRA_MINUTE_LEDS[3])
        });
        ipLabels.push_back(".");
      }
    }
  }
};
