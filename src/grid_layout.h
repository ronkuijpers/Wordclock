#pragma once

#include <Arduino.h>
#include "wordposition.h"

// Dimensions of the letter grid
const int GRID_WIDTH = 11;
const int GRID_HEIGHT = 11;
const int LED_COUNT_GRID = 146;
const int LED_COUNT_EXTRA = 15;  // minute LEDs
const int LED_COUNT_TOTAL = LED_COUNT_GRID + LED_COUNT_EXTRA;

enum class GridVariant : uint8_t {
  Classic = 0,
  VariantB,
  VariantC,
  VariantD,
};

struct GridVariantInfo {
  GridVariant variant;
  const char* key;   // short identifier, e.g. "classic"
  const char* label; // human-readable name for UI
};

// Active layout data
extern const char* const* LETTER_GRID;
extern const WordPosition* ACTIVE_WORDS;
extern size_t ACTIVE_WORD_COUNT;
extern const uint16_t* EXTRA_MINUTE_LEDS;
extern size_t EXTRA_MINUTE_LED_COUNT;

// Variant management helpers
GridVariant getActiveGridVariant();
bool setActiveGridVariant(GridVariant variant);
bool setActiveGridVariantById(uint8_t id);
bool setActiveGridVariantByKey(const char* key);
GridVariant gridVariantFromId(uint8_t id);
GridVariant gridVariantFromKey(const char* key);
uint8_t gridVariantToId(GridVariant variant);
const GridVariantInfo* getGridVariantInfos(size_t& count);
const GridVariantInfo* getGridVariantInfo(GridVariant variant);

// Lightweight lookup instead of unordered_map
const WordPosition* find_word(const char* name);
