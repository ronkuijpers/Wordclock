#include "grid_variants/en_v1.h"

// Placeholder: EN_V1 currently reuses the NL_V1 grid until a dedicated layout is supplied.
const uint16_t LED_COUNT_GRID_EN_V1 = 146;
const uint16_t LED_COUNT_EXTRA_EN_V1 = 15;
const uint16_t LED_COUNT_TOTAL_EN_V1 = LED_COUNT_GRID_EN_V1 + LED_COUNT_EXTRA_EN_V1;

const char* const LETTER_GRID_EN_V1[] = {
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

const uint16_t EXTRA_MINUTES_EN_V1[] = {
  static_cast<uint16_t>(LED_COUNT_GRID_EN_V1 + 7),
  static_cast<uint16_t>(LED_COUNT_GRID_EN_V1 + 9),
  static_cast<uint16_t>(LED_COUNT_GRID_EN_V1 + 11),
  static_cast<uint16_t>(LED_COUNT_GRID_EN_V1 + 13)
};

const WordPosition WORDS_EN_V1[] = {
  { "HET",         { 1, 2, 3 } },
  { "IS",          { 5, 6 } },
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

const size_t WORDS_EN_V1_COUNT = sizeof(WORDS_EN_V1) / sizeof(WORDS_EN_V1[0]);
const size_t EXTRA_MINUTES_EN_V1_COUNT = sizeof(EXTRA_MINUTES_EN_V1) / sizeof(EXTRA_MINUTES_EN_V1[0]);
