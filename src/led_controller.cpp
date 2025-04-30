#include "led_controller.h"
#include "config.h"
#include "log.h"
#include "led_state.h"


// Instantie van de NeoPixel-strip
static Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

void initLeds() {
    strip.begin();
    strip.setBrightness(ledState.getBrightness());
    strip.clear();
    strip.show();
}

void showLeds(const std::vector<uint16_t> &ledIndices) {
  strip.clear();
  for (uint16_t idx : ledIndices) {
    if (idx < strip.numPixels()) {
      // Gebruik de berekende RGB én W
      uint8_t r, g, b, w;
      ledState.getRGBW(r, g, b, w);
      strip.setPixelColor(idx, strip.Color(r, g, b, w));
    }
  }
  strip.setBrightness(ledState.getBrightness());
  strip.show();
}

