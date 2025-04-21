#include "led_controller.h"
#include "config.h"

// Instantie van de NeoPixel-strip
static Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void initLeds() {
    strip.begin();              // Start de strip
    strip.setBrightness(DEFAULT_BRIGHTNESS);
    strip.show();               // Zet alle LEDs uit
}

void showLeds(const std::vector<uint16_t> &ledIndices) {
    // Zet eerst alle LEDs uit
    for (uint16_t i = 0; i < strip.numPixels(); ++i) {
        strip.setPixelColor(i, 0);
    }
    // Schakel de gevraagde LEDs aan (bijv. in wit)
    for (uint16_t idx : ledIndices) {
        if (idx < strip.numPixels()) {
            strip.setPixelColor(idx, strip.Color(255, 255, 255));
        }
    }
    strip.show();
}
