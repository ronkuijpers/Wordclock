#include "led_controller.h"
#include "config.h"
#include "led_state.h"
#include "night_mode.h"

#include <vector>

// Instance of the NeoPixel strip
static Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

void initLeds() {
    strip.begin();
    uint8_t brightness = nightMode.applyToBrightness(ledState.getBrightness());
    strip.setBrightness(brightness);
    strip.clear();
    strip.show();
}

void showLeds(const std::vector<uint16_t> &ledIndices) {
  strip.clear();
  for (uint16_t idx : ledIndices) {
    if (idx < strip.numPixels()) {
      // Use the calculated RGB and W
      uint8_t r, g, b, w;
      ledState.getRGBW(r, g, b, w);
      strip.setPixelColor(idx, strip.Color(r, g, b, w));
    }
  }
  uint8_t brightness = nightMode.applyToBrightness(ledState.getBrightness());
  strip.setBrightness(brightness);
  strip.show();
}
