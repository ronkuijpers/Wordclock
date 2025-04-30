#pragma once

#include <unordered_map>
#include <string>
#include <Arduino.h>
#include "wordposition.h"

// Afmetingen van het lettergrid
const int GRID_WIDTH = 11;
const int GRID_HEIGHT = 11;
const int LED_COUNT_GRID = 146;
const int LED_COUNT_EXTRA = 15;  // minuten-leds
const int LED_COUNT_TOTAL = LED_COUNT_GRID + LED_COUNT_EXTRA;

// Aantal entries in WORDS[]
constexpr int WORDS_COUNT = 21;

// Externe variabelen
extern const WordPosition WORDS[WORDS_COUNT];
extern const char* LETTER_GRID[GRID_HEIGHT];
extern const int EXTRA_MINUTE_LEDS[4];

extern std::unordered_map<std::string, const WordPosition*> wordMap;
void initWordMap();