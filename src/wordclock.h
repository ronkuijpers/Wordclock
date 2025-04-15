#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "log.h"

// clockEnabled wordt extern gedefinieerd in Wordclock.ino
extern bool clockEnabled;

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

void wordclock_setup() {
  strip.begin();
  strip.setBrightness(DEFAULT_BRIGHTNESS);
  strip.clear();
  strip.show();
  logln("Wordclock setup complete");
}

void wordclock_loop() {
  if (!clockEnabled) {
    strip.clear();
    strip.show();
    return;
  }

  strip.clear();

  uint32_t color = strip.Color(255, 255, 255); // Wit (RGB)
  for (int i = 1; i < NUM_LEDS; i += 2) {
    strip.setPixelColor(i, color);
  }

  strip.show();

  // Eventueel een keer loggen
  static bool logged = false;
  if (!logged) {
    logln("Oneven LEDs opgelicht");
    logged = true;
  }
}




