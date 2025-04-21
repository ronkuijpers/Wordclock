#include "wordclock.h"

void wordclock_setup() {
  initLeds();
  logln("Wordclock setup complete");
}

void wordclock_loop() {
  if (!clockEnabled) {
    // klok uit: alles uit
    showLeds({});
    return;
  }

  // TEST: elke oneven LED
  std::vector<uint16_t> indices;
  for (uint16_t i = 1; i < NUM_LEDS; i += 2) {
    indices.push_back(i);
  }

  // Doorgeven aan de controller
  showLeds(indices);

  static bool logged = false;
  if (!logged) {
    logln("Oneven LEDs opgelicht");
    logged = true;
  }
}
