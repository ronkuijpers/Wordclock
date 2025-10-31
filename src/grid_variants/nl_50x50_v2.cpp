#include "grid_variants/nl_50x50_v2.h"

// Mirrors the NL_V4 layout for the 50x50 hardware variant.
const uint16_t LED_COUNT_GRID_NL_50x50_V2 = 128;
const uint16_t LED_COUNT_EXTRA_NL_50x50_V2 = 13;
const uint16_t LED_COUNT_TOTAL_NL_50x50_V2 = LED_COUNT_GRID_NL_50x50_V2 + LED_COUNT_EXTRA_NL_50x50_V2;

const char* const LETTER_GRID_NL_50x50_V2[] = {
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

const uint16_t EXTRA_MINUTES_NL_50x50_V2[] = {
  static_cast<uint16_t>(LED_COUNT_GRID_NL_50x50_V2 + 11),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_50x50_V2 + 9),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_50x50_V2 + 7),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_50x50_V2 + 5)
};

const WordPosition WORDS_NL_50x50_V2[] = {
  { "HET",         { 11, 10, 9 } },
  { "IS",          { 7, 6 } },
  { "VIJF_M",      { 37, 36, 35, 34 } },
  { "TIEN_M",      { 15, 16, 17, 18 } },
  { "OVER",        { 40, 41, 42, 43 } },
  { "VOOR",        { 60, 59, 58, 57 } },
  { "KWART",       { 31, 30, 29, 28, 27 } },
  { "HALF",        { 22, 29, 48, 55 } },
  { "UUR",         { 126, 127, 128 } },
  { "EEN",         { 115, 114, 113 } },
  { "TWEE",        { 88, 93, 114, 119 } },
  { "DRIE",        { 66, 67, 68, 69 } },
  { "VIER",        { 49, 54, 75, 80 } },
  { "VIJF",        { 120, 121, 122, 123 } },
  { "ZES",         { 72, 83, 98 } },
  { "ZEVEN",       { 72, 73, 74, 75, 76 } },
  { "ACHT",        { 108, 107, 106, 105 } },
  { "NEGEN",       { 113, 112, 111, 110, 109 } },
  { "TIEN",        { 87, 86, 85, 84 } },
  { "ELF",         { 73, 82, 99 } },
  { "TWAALF",      { 92, 93, 94, 95, 96, 97 } }
};

const size_t WORDS_NL_50x50_V2_COUNT = sizeof(WORDS_NL_50x50_V2) / sizeof(WORDS_NL_50x50_V2[0]);
const size_t EXTRA_MINUTES_NL_50x50_V2_COUNT = sizeof(EXTRA_MINUTES_NL_50x50_V2) / sizeof(EXTRA_MINUTES_NL_50x50_V2[0]);
