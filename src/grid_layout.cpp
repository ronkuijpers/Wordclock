#include "grid_layout.h"
#include <unordered_map>
#include <string>
#include <Arduino.h>


std::unordered_map<std::string, const WordPosition*> wordMap;

void initWordMap() {
  for (int i = 0; i < WORDS_COUNT; ++i) {
    wordMap[std::string(WORDS[i].word)] = &WORDS[i];
  }
}

// The grid layout (for reference or debugging only)
const char* LETTER_GRID[GRID_HEIGHT] = {
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
const int EXTRA_MINUTE_LEDS[4] = {
  LED_COUNT_GRID + 7,
  LED_COUNT_GRID + 9,
  LED_COUNT_GRID + 11,
  LED_COUNT_GRID + 13
};

// Mapping of words to their LED positions
const WordPosition WORDS[WORDS_COUNT] = {
  { "HET IS",      { 1, 2, 3, 5, 6 } },
  { "UUR",         { 138, 137, 136 } },
  { "VIJF OVER",   { 31, 32, 33, 34, 56, 55, 54, 53 } },
  { "TIEN OVER",   { 25, 24, 23, 22, 56, 55, 54, 53 } },
  { "KWART OVER",  { 37, 38, 39, 40, 41, 56, 55, 54, 53 } },
  { "VIJF VOOR",   { 31, 32, 33, 34, 64, 65, 66, 67 } },
  { "TIEN VOOR",   { 25, 24, 23, 22, 64, 65, 66, 67 } },
  { "KWART VOOR",  { 37, 38, 39, 40, 41, 64, 65, 66, 67 } },
  { "HALF",        { 18, 39, 48, 69 } },
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
