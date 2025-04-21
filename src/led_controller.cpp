#include "led_controller.h"
#include "config.h"
#include "log.h"

// Instantie van de NeoPixel-strip
static Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

// Externe kleurvariabelen
extern uint8_t currentR, currentG, currentB, currentW;

void initLeds() {
    strip.begin();
    strip.setBrightness(DEFAULT_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void showLeds(const std::vector<uint16_t> &ledIndices) {
    strip.clear();
    for (uint16_t idx : ledIndices) {
      if (idx < strip.numPixels()) {
        // Gebruik de berekende RGB én W
        strip.setPixelColor(idx,
          strip.Color(currentR,
                      currentG,
                      currentB,
                      currentW));
      }
    }
    strip.show();
  }

