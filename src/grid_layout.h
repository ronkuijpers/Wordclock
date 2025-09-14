#pragma once

#include <Arduino.h>
#include "wordposition.h"

// Dimensions of the letter grid
const int GRID_WIDTH = 11;
const int GRID_HEIGHT = 11;
const int LED_COUNT_GRID = 146;
const int LED_COUNT_EXTRA = 15;  // minute LEDs
const int LED_COUNT_TOTAL = LED_COUNT_GRID + LED_COUNT_EXTRA;

// Number of entries in WORDS[]
constexpr int WORDS_COUNT = 21;

// External variables
extern const WordPosition WORDS[WORDS_COUNT];
extern const char* const LETTER_GRID[GRID_HEIGHT];
extern const uint16_t EXTRA_MINUTE_LEDS[4];

// Lightweight lookup instead of unordered_map
const WordPosition* find_word(const char* name);
