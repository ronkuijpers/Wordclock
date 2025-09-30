#include "grid_variants/nl_v2.h"

// v2 is new lay-out in spiegelbeeld t.o.v. v1, elke bocht met 4 leds

const uint16_t LED_COUNT_GRID_NL_V2 = 145;
const uint16_t LED_COUNT_EXTRA_NL_V2 = 13;
const uint16_t LED_COUNT_TOTAL_NL_V2 = LED_COUNT_GRID_NL_V2 + LED_COUNT_EXTRA_NL_V2;

const char* const LETTER_GRID_NL_V2[] = {
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

const uint16_t EXTRA_MINUTES_NL_V2[] = {
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V2 + 13),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V2 + 11),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V2 + 9),
  static_cast<uint16_t>(LED_COUNT_GRID_NL_V2 + 7)
};

const WordPosition WORDS_NL_V2[] = {
  { "HET",         { 10, 9, 8 } },
  { "IS",          { 6, 5 } },
  { "VIJF_M",      { 40, 39, 38, 37 } },
  { "TIEN_M",      { 16, 17, 18, 19 } },
  { "OVER",        { 45, 46, 47, 48 } },
  { "VOOR",        { 67, 66, 65, 64 } },
  { "KWART",       { 34, 33, 32, 31, 30 } },
  { "HALF",        { 23, 32, 53, 62 } },
  { "UUR",         { 143, 144, 145 } },
  { "EEN",         { 130, 129, 128 } },
  { "TWEE",        { 99, 106, 129, 136 } },
  { "DRIE",        { 75, 76, 77, 78 } },
  { "VIER",        { 54, 61, 84, 91 } },
  { "VIJF",        { 137, 138, 139, 140 } },
  { "ZES",         { 81, 94, 111 } },
  { "ZEVEN",       { 81, 82, 83, 84, 85 } },
  { "ACHT",        { 123, 122, 121, 120 } },
  { "NEGEN",       { 128, 127, 126, 125, 124 } },
  { "TIEN",        { 98, 97, 96, 95 } },
  { "ELF",         { 82, 93, 112 } },
  { "TWAALF",      { 105, 106, 107, 108, 109, 110 } }
};

const size_t WORDS_NL_V2_COUNT = sizeof(WORDS_NL_V2) / sizeof(WORDS_NL_V2[0]);
const size_t EXTRA_MINUTES_NL_V2_COUNT = sizeof(EXTRA_MINUTES_NL_V2) / sizeof(EXTRA_MINUTES_NL_V2[0]);
