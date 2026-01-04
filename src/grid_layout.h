#pragma once

#include <Arduino.h>
#include "wordposition.h"

// Dimensions of the letter grid
const int GRID_WIDTH = 11;
const int GRID_HEIGHT = 11;

enum class GridVariant : uint8_t {
  NL_20x20_V1 = 0,
  EN_20x20_V1,
};

struct GridVariantInfo {
  GridVariant variant;
  const char* key;       // identifier like "NL_V1"
  const char* label;     // human-readable name for UI
  const char* language;  // ISO language code, e.g. "nl"
  const char* version;   // version string, e.g. "v1"
};

// Active layout data
extern const char* const* LETTER_GRID;
extern const WordPosition* ACTIVE_WORDS;
extern size_t ACTIVE_WORD_COUNT;
extern const uint16_t* NO_TIME_INDICATOR_LEDS;
extern size_t NO_TIME_INDICATOR_LED_COUNT;

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

// Active LED count
uint16_t getActiveLedCountTotal();
