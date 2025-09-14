
#ifndef LED_STATE_H
#define LED_STATE_H

#include <Preferences.h>

class LedState {
public:
  void begin() {
    prefs.begin("led", false);
    red   = prefs.getUChar("r", 0);
    green = prefs.getUChar("g", 0);
    blue  = prefs.getUChar("b", 0);
    white = prefs.getUChar("w", 255);
    brightness = prefs.getUChar("br", 128);
    prefs.end();
  }

  void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    red = r; green = g; blue = b;
    white = (r == 255 && g == 255 && b == 255) ? 255 : 0;
    if (white) { red = green = blue = 0; }

    prefs.begin("led", false);
    prefs.putUChar("r", red);
    prefs.putUChar("g", green);
    prefs.putUChar("b", blue);
    prefs.putUChar("w", white);
    prefs.end();
  }

  void setBrightness(uint8_t b) {
    brightness = b;
    prefs.begin("led", false);
    prefs.putUChar("br", brightness);
    prefs.end();
  }

  uint8_t getBrightness() const { return brightness; }

  void getRGBW(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) const {
    r = red; g = green; b = blue; w = white;
  }

private:
  uint8_t red = 0, green = 0, blue = 0, white = 255;
  uint8_t brightness = 128;
  Preferences prefs;
};

extern LedState ledState;

#endif // LED_STATE_H
