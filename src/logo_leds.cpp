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
constexpr uint8_t LOGO_DATA_PINS[LOGO_STRIP_COUNT] = { LOGO_DATA_PIN_1, LOGO_DATA_PIN_2 };
static_assert(LOGO_STRIP_COUNT == (sizeof(LOGO_DATA_PINS) / sizeof(LOGO_DATA_PINS[0])),
              "LOGO_DATA_PINS must match LOGO_STRIP_COUNT");
constexpr uint16_t LOGO_LED_COUNTS[LOGO_STRIP_COUNT] = { LOGO_LED_COUNT_PIN18, LOGO_LED_COUNT_PIN19 };
constexpr const char* PREF_NAMESPACE = "logo";
constexpr const char* PREF_KEY_BRIGHTNESS = "br";

// Hardcoded colors for the logo strips (RRGGBB). Edit this array to change the logo palette.
// LEDs 0-12 = strip on LOGO_DATA_PIN_1 (13 LEDs), LEDs 13-24 = strip on LOGO_DATA_PIN_2 (12 LEDs).
// Original palette (for reference):
// static constexpr std::array<uint32_t, LOGO_LED_TOTAL> DEFAULT_LOGO_COLORS = {
//   // Strip 1 (13 LEDs)
//   0x000000, 0xFFFFFF, 0xF2B40F, 0xFFFFFF, 0xFFFFFF,
//   0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
//   0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
//   // Strip 2 (12 LEDs)
//   0x000000, 0xFFFFFF, 0xF2B40F, 0xFFFFFF, 0xFFFFFF,
//   0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
//   0xFFFFFF, 0xFFFFFF
// };
static constexpr std::array<uint32_t, LOGO_LED_TOTAL> DEFAULT_LOGO_COLORS = {
  // Strip 1 (13 LEDs)
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
  // Strip 2 (12 LEDs)
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
  0xFFFFFF, 0xFFFFFF
};

std::array<uint32_t, LOGO_LED_TOTAL> g_colors{};
uint8_t g_lastBrightness = 0;
bool g_haveBrightness = false;
uint8_t g_logoBrightness = 64;
Preferences g_prefs;

#ifndef PIO_UNIT_TESTING
Adafruit_NeoPixel g_logoStrips[LOGO_STRIP_COUNT];
std::array<bool, LOGO_STRIP_COUNT> g_stripConfigured{};

void ensureStripConfigured(size_t stripIdx) {
  if (stripIdx >= LOGO_STRIP_COUNT) return;
  if (g_stripConfigured[stripIdx]) return;
  g_logoStrips[stripIdx].updateType(NEO_GRBW + NEO_KHZ800);
  g_logoStrips[stripIdx].setPin(LOGO_DATA_PINS[stripIdx]);
  g_logoStrips[stripIdx].updateLength(LOGO_LED_COUNTS[stripIdx]);
  g_logoStrips[stripIdx].begin();
  g_logoStrips[stripIdx].clear();
  g_logoStrips[stripIdx].show();
  g_stripConfigured[stripIdx] = true;
}

constexpr bool isDisabledIndex(uint16_t) { return false; }

void pushToStrips(uint8_t brightness) {
  uint16_t base = 0;
  for (size_t s = 0; s < LOGO_STRIP_COUNT; ++s) {
    ensureStripConfigured(s);
    auto& strip = g_logoStrips[s];
    strip.clear();
    uint16_t len = LOGO_LED_COUNTS[s];
    for (uint16_t i = 0; i < len; ++i) {
      uint16_t globalIndex = static_cast<uint16_t>(base + i);
      uint32_t rgb = (g_colors[globalIndex] & 0xFFFFFF);
      uint8_t r = (rgb >> 16) & 0xFF;
      uint8_t g = (rgb >> 8) & 0xFF;
      uint8_t b = rgb & 0xFF;
      strip.setPixelColor(i, strip.Color(r, g, b, 0));
    }
    strip.setBrightness(brightness);
    strip.show();
    base = static_cast<uint16_t>(base + len);
  }
}
#else
std::vector<uint32_t> g_lastShown;
#endif

uint8_t currentBrightness() {
  return nightMode.applyToBrightness(g_logoBrightness);
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
  g_colors = DEFAULT_LOGO_COLORS;
  // Load logo-specific brightness (fallback to main brightness)
  g_prefs.begin(PREF_NAMESPACE, true);
  g_logoBrightness = g_prefs.getUChar(PREF_KEY_BRIGHTNESS, ledState.getBrightness());
  g_prefs.end();
#ifndef PIO_UNIT_TESTING
  g_stripConfigured.fill(false);
#else
  g_lastShown.clear();
#endif
  uint8_t brightness = currentBrightness();
  apply(brightness);
  String counts = String(LOGO_LED_COUNTS[0]);
  for (size_t i = 1; i < LOGO_STRIP_COUNT; ++i) {
    counts += ", ";
    counts += LOGO_LED_COUNTS[i];
  }
  logInfo(String("🪧 Logo strips initialized (total ") + LOGO_LED_TOTAL +
          " LEDs; pins " + LOGO_DATA_PIN_1 + ", " + LOGO_DATA_PIN_2 +
          "; per strip: [" + counts + "])");
}

bool logoSetColor(uint16_t index, uint32_t rgb) {
  if (index >= LOGO_LED_TOTAL) return false;
  g_colors[index] = isDisabledIndex(index) ? 0 : (rgb & 0xFFFFFF);
  apply(currentBrightness());
  return true;
}

bool logoSetColors(const std::vector<uint32_t>& colors) {
  if (colors.size() != LOGO_LED_TOTAL) return false;
  for (size_t i = 0; i < LOGO_LED_TOTAL; ++i) {
    g_colors[i] = isDisabledIndex(static_cast<uint16_t>(i)) ? 0 : (colors[i] & 0xFFFFFF);
  }
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

void setLogoBrightness(uint8_t level) {
  if (level > 255) level = 255;
  if (g_logoBrightness == level) return;
  g_logoBrightness = level;
  g_prefs.begin(PREF_NAMESPACE, false);
  g_prefs.putUChar(PREF_KEY_BRIGHTNESS, g_logoBrightness);
  g_prefs.end();
  apply(currentBrightness());
}

uint8_t getLogoBrightness() {
  return g_logoBrightness;
}
