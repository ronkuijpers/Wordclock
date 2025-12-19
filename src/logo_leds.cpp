#include "logo_leds.h"

#include <array>
#include <Preferences.h>
#ifndef PIO_UNIT_TESTING
#include <Adafruit_NeoPixel.h>
#endif

#include "config.h"
#include "led_state.h"
#include "log.h"
#include "night_mode.h"

namespace {
constexpr const char* PREF_NAMESPACE = "logo";
constexpr const char* PREF_KEY_COLORS = "colors";
constexpr uint8_t LOGO_DATA_PINS[LOGO_STRIP_COUNT] = { LOGO_DATA_PIN_1, LOGO_DATA_PIN_2 };
static_assert(LOGO_STRIP_COUNT == (sizeof(LOGO_DATA_PINS) / sizeof(LOGO_DATA_PINS[0])),
              "LOGO_DATA_PINS must match LOGO_STRIP_COUNT");

std::array<uint32_t, LOGO_LED_TOTAL> g_colors{};
uint8_t g_lastBrightness = 0;
bool g_haveBrightness = false;
Preferences g_prefs;

#ifndef PIO_UNIT_TESTING
Adafruit_NeoPixel g_logoStrips[LOGO_STRIP_COUNT];
std::array<bool, LOGO_STRIP_COUNT> g_stripConfigured{};

void ensureStripConfigured(size_t stripIdx) {
  if (stripIdx >= LOGO_STRIP_COUNT) return;
  if (g_stripConfigured[stripIdx]) return;
  g_logoStrips[stripIdx].updateType(NEO_GRBW + NEO_KHZ800);
  g_logoStrips[stripIdx].setPin(LOGO_DATA_PINS[stripIdx]);
  g_logoStrips[stripIdx].updateLength(LOGO_LED_PER_STRIP);
  g_logoStrips[stripIdx].begin();
  g_logoStrips[stripIdx].clear();
  g_logoStrips[stripIdx].show();
  g_stripConfigured[stripIdx] = true;
}

void pushToStrips(uint8_t brightness) {
  for (size_t s = 0; s < LOGO_STRIP_COUNT; ++s) {
    ensureStripConfigured(s);
    auto& strip = g_logoStrips[s];
    strip.clear();
    for (uint16_t i = 0; i < LOGO_LED_PER_STRIP; ++i) {
      uint16_t globalIndex = static_cast<uint16_t>(s * LOGO_LED_PER_STRIP + i);
      uint32_t rgb = g_colors[globalIndex] & 0xFFFFFF;
      uint8_t r = (rgb >> 16) & 0xFF;
      uint8_t g = (rgb >> 8) & 0xFF;
      uint8_t b = rgb & 0xFF;
      strip.setPixelColor(i, strip.Color(r, g, b, 0));
    }
    strip.setBrightness(brightness);
    strip.show();
  }
}
#else
std::vector<uint32_t> g_lastShown;
#endif

uint8_t currentBrightness() {
  return nightMode.applyToBrightness(ledState.getBrightness());
}

void loadColors() {
  g_colors.fill(0);
  g_prefs.begin(PREF_NAMESPACE, true);
  size_t expected = LOGO_LED_TOTAL * sizeof(uint32_t);
  size_t stored = g_prefs.getBytesLength(PREF_KEY_COLORS);
  if (stored == expected) {
    g_prefs.getBytes(PREF_KEY_COLORS, g_colors.data(), expected);
  }
  g_prefs.end();
}

void persistColors() {
  g_prefs.begin(PREF_NAMESPACE, false);
  g_prefs.putBytes(PREF_KEY_COLORS, g_colors.data(), g_colors.size() * sizeof(uint32_t));
  g_prefs.end();
}

void apply(uint8_t brightness) {
#ifndef PIO_UNIT_TESTING
  pushToStrips(brightness);
#else
  g_lastShown.assign(g_colors.begin(), g_colors.end());
#endif
  g_lastBrightness = brightness;
  g_haveBrightness = true;
}

}  // namespace

void initLogoLeds() {
  loadColors();
#ifndef PIO_UNIT_TESTING
  g_stripConfigured.fill(false);
#else
  g_lastShown.clear();
#endif
  uint8_t brightness = currentBrightness();
  apply(brightness);
  logInfo(String("🪧 Logo strips initialized (") + LOGO_STRIP_COUNT + " x " + LOGO_LED_PER_STRIP +
          " LEDs; pins " + LOGO_DATA_PIN_1 + ", " + LOGO_DATA_PIN_2 + ")");
}

bool logoSetColor(uint16_t index, uint32_t rgb) {
  if (index >= LOGO_LED_TOTAL) return false;
  g_colors[index] = rgb & 0xFFFFFF;
  persistColors();
  apply(currentBrightness());
  return true;
}

bool logoSetColors(const std::vector<uint32_t>& colors) {
  if (colors.size() != LOGO_LED_TOTAL) return false;
  for (size_t i = 0; i < LOGO_LED_TOTAL; ++i) {
    g_colors[i] = colors[i] & 0xFFFFFF;
  }
  persistColors();
  apply(currentBrightness());
  return true;
}

std::vector<uint32_t> logoGetColors() {
  return std::vector<uint32_t>(g_colors.begin(), g_colors.end());
}

void logoTick() {
  uint8_t brightness = currentBrightness();
  if (!g_haveBrightness || brightness != g_lastBrightness) {
    apply(brightness);
  }
}
