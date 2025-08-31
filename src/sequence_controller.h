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
    IP_PAUSE,
    DONE
  };

  StartupSequence() : state(SWEEP), index(0), lastUpdate(0), step(0) {}

  void start() {
    state = SWEEP;
    index = 0;
    step = 0;
    lastUpdate = millis();
    logDebug("🔁 Startup: Sweep begint");
  }

  void update() {
    unsigned long now = millis();

    switch (state) {
      case SWEEP:
        if (now - lastUpdate >= 62 && index < NUM_LEDS) {
          showLeds({(uint16_t)index});
          index++;
          lastUpdate = now;
          if (index >= NUM_LEDS) {
            state = IP_DIGITS;
            logDebug("📡 Startup: IP-adres animatie");
            prepareIPSequence();
            step = 0;
            lastUpdate = now;
          }
        }
        break;

      case IP_DIGITS:
        if (now - lastUpdate >= 3000 && step < ipSequence.size()) {
          showLeds(ipSequence[step]);
          logDebug(String("🔢 IP-deel getoond: ") + ipLabels[step]);
          step++;
          lastUpdate = now;
          if (step >= ipSequence.size()) {
            state = IP_PAUSE;
            lastUpdate = now;
            showLeds({});
            logDebug("⏳ Startup: pauze na IP-weergave");
          }
        }
        break;

      case IP_PAUSE:
        if (now - lastUpdate >= 5000) {
          state = DONE;
          showLeds({});
          logInfo("✅ Startup voltooid");
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
          case '0': word = "TWAALF"; break;
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
