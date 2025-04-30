#pragma once
#include "led_controller.h"
#include "grid_layout.h"
#include <vector>

void startupSequence() {
  // 1 voor 1 leds aanzetten
  for (uint16_t i = 0; i < LED_COUNT_TOTAL; i++) {
    std::vector<uint16_t> singleLed = {i};
    showLeds(singleLed);
    delay(50);
  }
  delay(500);

  // 1 voor 1 leds uitzetten
  for (uint16_t i = 0; i < LED_COUNT_TOTAL; i++) {
    showLeds({});
    delay(50);
  }
}
