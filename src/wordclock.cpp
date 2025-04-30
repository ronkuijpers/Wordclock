#include "wordclock.h"
#include "time_mapper.h"
#include "led_state.h"



void wordclock_setup() {
  ledState.begin();
  initLeds();
  logln("Wordclock setup complete");
}

void wordclock_loop() {
  if (!clockEnabled) {
    // klok uit: alles uit
    showLeds({});
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    logln("❌ Kon tijd niet ophalen");
    return;
  }
  
  int currentMinute = timeinfo.tm_min;
  
  static int lastDisplayedMinute = -1;
  if (currentMinute != lastDisplayedMinute) {
    lastDisplayedMinute = currentMinute;
  
    // Bepaal welke LEDs aan moeten
    std::vector<uint16_t> indices = get_led_indices_for_time(&timeinfo);
    showLeds(indices);

    // Log welke LED-indexen zijn aangezet
    String ledList = "";
    for (uint16_t idx : indices) {
      ledList += String(idx) + ",";
    }
    logln(String("LED indices: ") + ledList);
  }
}
