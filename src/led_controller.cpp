#include "led_controller.h"
#include "config.h"
#include "grid_layout.h"
#include "led_state.h"
#include "night_mode.h"

#include <vector>

#ifndef PIO_UNIT_TESTING
// Instance of the NeoPixel strip; length is synchronized with the active grid variant.
static Adafruit_NeoPixel strip;
static uint16_t activeStripLength = 0;

static void ensureStripLength() {
  uint16_t required = getActiveLedCountTotal();
  if (required == 0) {
    required = 1; // keep strip functional even if layout is missing
  }
  if (required != activeStripLength) {
    activeStripLength = required;
    strip.updateType(NEO_GRBW + NEO_KHZ800);
    strip.setPin(DATA_PIN);
    strip.updateLength(activeStripLength);
    strip.begin();
    strip.clear();
    strip.show();
  }
}
#else
static std::vector<uint16_t> lastShown;
#endif

void initLeds() {
#ifndef PIO_UNIT_TESTING
  ensureStripLength();
  uint8_t brightness = nightMode.applyToBrightness(ledState.getBrightness());
  strip.setBrightness(brightness);
  strip.clear();
  strip.show();
#else
  lastShown.clear();
#endif
}

void showLeds(const std::vector<uint16_t> &ledIndices) {
#ifndef PIO_UNIT_TESTING
  ensureStripLength();
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
#else
  lastShown = ledIndices;
#endif
}

#ifdef PIO_UNIT_TESTING
const std::vector<uint16_t>& test_getLastShownLeds() {
  return lastShown;
}

void test_clearLastShownLeds() {
  lastShown.clear();
}
#endif
