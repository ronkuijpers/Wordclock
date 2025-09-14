#include "grid_layout.h"
#include <Arduino.h>

// The grid layout (for reference or debugging only)
const char* const LETTER_GRID[GRID_HEIGHT] = {
  "HETBISWYBRC", // 1..11
  "RTIENMMUHLC", // 26..16
  "VIJFCWKWART", // 31..41
  "OVERXTTXLVB", // 56..46
  "QKEVOORTFIG", // 61..71
  "DRIEKBZEVEN", // 86..76
  "VTTIENELNRC", // 91..101
  "TWAALFSFRSF", // 116..106
  "EENEGENACHT", // 121..131
  "XEVIJFJXUUR", // 146..136
  "..-.-.-.-.."  // 151..161
};

// The indices of the four extra minute LEDs (after the main grid)
const uint16_t EXTRA_MINUTE_LEDS[4] = {
  static_cast<uint16_t>(LED_COUNT_GRID + 7),
  static_cast<uint16_t>(LED_COUNT_GRID + 9),
  static_cast<uint16_t>(LED_COUNT_GRID + 11),
  static_cast<uint16_t>(LED_COUNT_GRID + 13)
};

// Mapping of words to their LED positions
const WordPosition WORDS[WORDS_COUNT] = {
  // Split HET and IS so they can animate separately
  { "HET",         { 1, 2, 3 } },
  { "IS",          { 5, 6 } },

  // Minute words split into separate parts
  { "VIJF_M",      { 31, 32, 33, 34 } },  // minutes "VIJF"
  { "TIEN_M",      { 25, 24, 23, 22 } },  // minutes "TIEN"
  { "OVER",        { 56, 55, 54, 53 } },
  { "VOOR",        { 64, 65, 66, 67 } },
  { "KWART",       { 37, 38, 39, 40, 41 } },


  // Other existing words
  { "HALF",        { 18, 39, 48, 69 } },
  { "UUR",         { 138, 137, 136 } },

  // Hour words (note: these remain unchanged)
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

const WordPosition* find_word(const char* name) {
  for (int i = 0; i < WORDS_COUNT; ++i) {
    if (strcmp(WORDS[i].word, name) == 0) return &WORDS[i];
  }
  return nullptr;
}
