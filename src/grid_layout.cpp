#include "grid_layout.h"

#include <Arduino.h>
#include <string.h>

#include "grid_variants/en_20x20_v1.h"
#include "grid_variants/nl_20x20_v1.h"

namespace {

struct GridVariantData {
  GridVariant variant;
  const char* key;
  const char* label;
  const char* language;
  const char* version;
  uint16_t ledCountTotal;
  const char* const* letterGrid;
  const WordPosition* words;
  size_t wordCount;
  const uint16_t* noTimeLeds;
  size_t noTimeCount;
};

// Helper to compute array length at compile time
template <typename T, size_t N>
constexpr size_t countof(const T (&)[N]) { return N; }

static const GridVariantData GRID_VARIANTS[] = {
  { GridVariant::NL_20x20_V1, "NL_20x20_V1", "Nederlands 20x20 V1", "nl", "v1", LED_COUNT_TOTAL_NL_20x20_V1, LETTER_GRID_NL_20x20_V1, WORDS_NL_20x20_V1, WORDS_NL_20x20_V1_COUNT, NO_TIME_INDICATOR_LEDS_NL_20x20_V1, NO_TIME_INDICATOR_LEDS_NL_20x20_V1_COUNT },
  { GridVariant::EN_20x20_V1, "EN_20x20_V1", "English 20x20 V1", "en", "v1", LED_COUNT_TOTAL_EN_20x20_V1, LETTER_GRID_EN_20x20_V1, WORDS_EN_20x20_V1, WORDS_EN_20x20_V1_COUNT, NO_TIME_INDICATOR_LEDS_EN_20x20_V1, NO_TIME_INDICATOR_LEDS_EN_20x20_V1_COUNT }
};

static const GridVariantData* activeVariant = &GRID_VARIANTS[0];

void applyActiveVariant(const GridVariantData* data) {
  activeVariant = data;
  LETTER_GRID = data->letterGrid;
  ACTIVE_WORDS = data->words;
  ACTIVE_WORD_COUNT = data->wordCount;
  NO_TIME_INDICATOR_LEDS = data->noTimeLeds;
  NO_TIME_INDICATOR_LED_COUNT = data->noTimeCount;
}

const GridVariantData* findVariant(GridVariant variant) {
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    if (GRID_VARIANTS[i].variant == variant) {
      return &GRID_VARIANTS[i];
    }
  }
  return nullptr;
}

const GridVariantData* findVariantByKey(const char* key) {
  if (!key) return nullptr;
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    if (strcmp(GRID_VARIANTS[i].key, key) == 0) {
      return &GRID_VARIANTS[i];
    }
  }
  return nullptr;
}

} // namespace

// Public state - default to NL 20x20 V1
const char* const* LETTER_GRID = LETTER_GRID_NL_20x20_V1;
const WordPosition* ACTIVE_WORDS = WORDS_NL_20x20_V1;
size_t ACTIVE_WORD_COUNT = WORDS_NL_20x20_V1_COUNT;
const uint16_t* NO_TIME_INDICATOR_LEDS = NO_TIME_INDICATOR_LEDS_NL_20x20_V1;
size_t NO_TIME_INDICATOR_LED_COUNT = NO_TIME_INDICATOR_LEDS_NL_20x20_V1_COUNT;

GridVariant getActiveGridVariant() {
  return activeVariant->variant;
}

bool setActiveGridVariant(GridVariant variant) {
  const GridVariantData* data = findVariant(variant);
  if (!data) return false;
  applyActiveVariant(data);
  return true;
}

bool setActiveGridVariantById(uint8_t id) {
  if (id >= countof(GRID_VARIANTS)) return false;
  applyActiveVariant(&GRID_VARIANTS[id]);
  return true;
}

bool setActiveGridVariantByKey(const char* key) {
  const GridVariantData* data = findVariantByKey(key);
  if (!data) return false;
  applyActiveVariant(data);
  return true;
}

GridVariant gridVariantFromId(uint8_t id) {
  if (id >= countof(GRID_VARIANTS)) {
    return GRID_VARIANTS[0].variant;
  }
  return GRID_VARIANTS[id].variant;
}

GridVariant gridVariantFromKey(const char* key) {
  const GridVariantData* data = findVariantByKey(key);
  if (!data) {
    return GRID_VARIANTS[0].variant;
  }
  return data->variant;
}

uint8_t gridVariantToId(GridVariant variant) {
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    if (GRID_VARIANTS[i].variant == variant) {
      return static_cast<uint8_t>(i);
    }
  }
  return 0;
}

uint16_t getActiveLedCountTotal() {
  return activeVariant ? activeVariant->ledCountTotal : 0;
}

const GridVariantInfo* getGridVariantInfos(size_t& count) {
  static GridVariantInfo infos[countof(GRID_VARIANTS)];
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    infos[i].variant = GRID_VARIANTS[i].variant;
    infos[i].key = GRID_VARIANTS[i].key;
    infos[i].label = GRID_VARIANTS[i].label;
    infos[i].language = GRID_VARIANTS[i].language;
    infos[i].version = GRID_VARIANTS[i].version;
  }
  count = countof(GRID_VARIANTS);
  return infos;
}

const GridVariantInfo* getGridVariantInfo(GridVariant variant) {
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    if (GRID_VARIANTS[i].variant == variant) {
      static GridVariantInfo info;
      info.variant = GRID_VARIANTS[i].variant;
      info.key = GRID_VARIANTS[i].key;
      info.label = GRID_VARIANTS[i].label;
      info.language = GRID_VARIANTS[i].language;
      info.version = GRID_VARIANTS[i].version;
      return &info;
    }
  }
  return nullptr;
}

const WordPosition* find_word(const char* name) {
  if (!name) return nullptr;
  for (size_t i = 0; i < ACTIVE_WORD_COUNT; ++i) {
    if (strcmp(ACTIVE_WORDS[i].word, name) == 0) {
      return &ACTIVE_WORDS[i];
    }
  }
  return nullptr;
}
