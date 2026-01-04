#include "grid_variants/en_20x20_v1.h"

// English 20x20 V1 layout - no HET/IS words
const uint16_t LED_COUNT_TOTAL_EN_20x20_V1 = 146;

const char* const LETTER_GRID_EN_20x20_V1[] = {
  "HETBISWYBRC",
  "RTIENMMUHLC",
  "VIJFCWKWART",
  "OVERXTTXLVB",
  "QKEVOORTFIG",
  "DRIEKBZEVEN",
  "VTTIENELNRC",
  "TWAALFSFRSF",
  "EENEGENACHT",
  "XEVIJFJXUUR",
  "..-.-.-.-.."
};

const WordPosition WORDS_EN_20x20_V1[] = {
  // No HET/IS in 20x20 grid
  { "VIJF_M",      { 31, 32, 33, 34 } },
  { "TIEN_M",      { 25, 24, 23, 22 } },
  { "OVER",        { 56, 55, 54, 53 } },
  { "VOOR",        { 64, 65, 66, 67 } },
  { "KWART",       { 37, 38, 39, 40, 41 } },
  { "HALF",        { 18, 39, 48, 69 } },
  { "UUR",         { 138, 137, 136 } },
  { "EEN",         { 121, 122, 123 } },
  { "TWEE",        { 92, 115, 122, 145 } },
  { "DRIE",        { 86, 85, 84, 83 } },
  { "VIER",        { 47, 70, 77, 100 } },
  { "VIJF",        { 144, 143, 142, 141 } },
  { "ZES",         { 80, 97, 110 } },
  { "ZEVEN",       { 80, 79, 78, 77, 76 } },
  { "ACHT",        { 128, 129, 130, 131 } },
  { "NEGEN",       { 123, 124, 125, 126, 127 } },
  { "TIEN",        { 93, 94, 95, 96 } },
  { "ELF",         { 79, 98, 109 } },
  { "TWAALF",      { 116, 115, 114, 113, 112, 111 } }
};

const size_t WORDS_EN_20x20_V1_COUNT = sizeof(WORDS_EN_20x20_V1) / sizeof(WORDS_EN_20x20_V1[0]);

// LEDs to blink when no time is available (e.g. NTP sync failed)
// Define 4 corner LEDs or any LEDs you want to use as indicator
const uint16_t NO_TIME_INDICATOR_LEDS_EN_20x20_V1[] = {
  1, 9, 97, 105  // Example: 4 corner LEDs (top-left, top-right, bottom-left, bottom-right)
};
const size_t NO_TIME_INDICATOR_LEDS_EN_20x20_V1_COUNT = sizeof(NO_TIME_INDICATOR_LEDS_EN_20x20_V1) / sizeof(NO_TIME_INDICATOR_LEDS_EN_20x20_V1[0]);
