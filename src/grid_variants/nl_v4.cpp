#include "grid_variants/nl_v4.h"

// Placeholder: NL_V4 currently reuses the NL_V1 grid until a dedicated layout is supplied.
const uint16_t LED_COUNT_GRID_NL_V4 = 137;
const uint16_t LED_COUNT_EXTRA_NL_V4 = 14;
const uint16_t LED_COUNT_TOTAL_NL_V4 = LED_COUNT_GRID_NL_V4 + LED_COUNT_EXTRA_NL_V4;

const char* const LETTER_GRID_NL_V4[] = {
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

const uint16_t EXTRA_MINUTES_NL_V4[] = {
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V4 + 6),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V4 + 8),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V4 + 10),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V4 + 12)
};

const WordPosition WORDS_NL_V4[] = {
  { "HET",         { 1, 2, 3 } },
  { "IS",          { 5, 6 } },
  { "VIJF_M",      { 29, 30, 31, 32 } },
  { "TIEN_M",      { 24, 23, 22, 21 } },
  { "OVER",        { 53, 52, 51, 50 } },
  { "VOOR",        { 60, 61, 62, 63 } },
  { "KWART",       { 35, 36, 37, 38, 39 } },
  { "HALF",        { 17, 37, 45, 65 } },
  { "UUR",         { 129, 128, 127 } },
  { "EEN",         { 113, 114, 115 } },
  { "TWEE",        { 86, 108, 114, 136 } },
  { "DRIE",        { 81, 80, 79, 78 } },
  { "VIER",        { 44, 66, 72, 94 } },
  { "VIJF",        { 135, 134, 133, 132 } },
  { "ZES",         { 75, 91, 103 } },
  { "ZEVEN",       { 75, 74, 73, 72, 71 } },
  { "ACHT",        { 120, 121, 122, 123 } },
  { "NEGEN",       { 115, 116, 117, 118, 119 } },
  { "TIEN",        { 87, 88, 89, 90 } },
  { "ELF",         { 74, 92, 102 } },
  { "TWAALF",      { 109, 108, 107, 106, 105, 104 } }
};

const size_t WORDS_NL_V4_COUNT = sizeof(WORDS_NL_V4) / sizeof(WORDS_NL_V4[0]);
const size_t EXTRA_MINUTES_NL_V4_COUNT = sizeof(EXTRA_MINUTES_NL_V4) / sizeof(EXTRA_MINUTES_NL_V4[0]);
