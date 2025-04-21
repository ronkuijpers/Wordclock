#include "led_controller.h"
#include "grid_layout.h"

#define DATA_PIN 5
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 64

CRGB leds[LED_COUNT_TOTAL];

void initLeds() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, LED_COUNT_TOTAL);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void showLeds(const std::vector<int>& indices, CRGB color) {
  FastLED.clear();
  for (int i : indices) {
    if (i >= 0 && i < LED_COUNT_TOTAL) {
      leds[i] = color;
    }
  }
  FastLED.show();
}
