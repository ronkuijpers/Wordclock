#include "wordclock.h"
#include "time_mapper.h"
#include "led_state.h"



void wordclock_setup() {
  ledState.begin();
  initLeds();
  logInfo("Wordclock setup complete");
}

void wordclock_loop() {
  if (!clockEnabled) {
    // clock off: clear all LEDs
    showLeds({});
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    logError("❌ Kon tijd niet ophalen");
    return;
  }
  
  int currentMinute = timeinfo.tm_min;
  
  static int lastDisplayedMinute = -1;
  if (currentMinute != lastDisplayedMinute) {
    lastDisplayedMinute = currentMinute;
  
    // Determine which LEDs should be on
    std::vector<uint16_t> indices = get_led_indices_for_time(&timeinfo);
    showLeds(indices);

    // Log which LED indices were activated
    String ledList = "";
    for (uint16_t idx : indices) {
      ledList += String(idx) + ",";
    }
    logDebug(String("LED indices: ") + ledList);
  }
}
