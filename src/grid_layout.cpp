#include "grid_layout.h"

#include <Arduino.h>
#include <string.h>

namespace {

struct GridVariantData {
  GridVariant variant;
  const char* key;
  const char* label;
  const char* const* letterGrid;
  size_t letterRows;
  const WordPosition* words;
  size_t wordCount;
  const uint16_t* minuteLeds;
  size_t minuteCount;
};

// Helper to compute array length at compile time
template <typename T, size_t N>
constexpr size_t countof(const T (&)[N]) { return N; }

// --- Grid data definitions -------------------------------------------------
// Add new variants by duplicating one of the blocks below and adjusting the
// letter grid and word positions. Each variant must keep the same vocabulary
// (word names) so existing time mapping logic continues to work.

// Classic layout (current production grid)
static const char* const LETTER_GRID_CLASSIC[GRID_HEIGHT] = {
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

static const uint16_t EXTRA_MINUTES_CLASSIC[] = {
  static_cast<uint16_t>(LED_COUNT_GRID + 7),
  static_cast<uint16_t>(LED_COUNT_GRID + 9),
  static_cast<uint16_t>(LED_COUNT_GRID + 11),
  static_cast<uint16_t>(LED_COUNT_GRID + 13)
};

static const WordPosition WORDS_CLASSIC[] = {
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

// Placeholder variants (copy of classic). Replace LETTER_GRID_*,
// EXTRA_MINUTES_* and WORDS_* with the correct mappings for each clock type.
static const char* const LETTER_GRID_VARIANT_B[GRID_HEIGHT] = {
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

static const uint16_t EXTRA_MINUTES_VARIANT_B[] = {
  static_cast<uint16_t>(LED_COUNT_GRID + 7),
  static_cast<uint16_t>(LED_COUNT_GRID + 9),
  static_cast<uint16_t>(LED_COUNT_GRID + 11),
  static_cast<uint16_t>(LED_COUNT_GRID + 13)
};

static const WordPosition WORDS_VARIANT_B[] = {
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

static const char* const LETTER_GRID_VARIANT_C[GRID_HEIGHT] = {
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

static const uint16_t EXTRA_MINUTES_VARIANT_C[] = {
  static_cast<uint16_t>(LED_COUNT_GRID + 7),
  static_cast<uint16_t>(LED_COUNT_GRID + 9),
  static_cast<uint16_t>(LED_COUNT_GRID + 11),
  static_cast<uint16_t>(LED_COUNT_GRID + 13)
};

static const WordPosition WORDS_VARIANT_C[] = {
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

static const char* const LETTER_GRID_VARIANT_D[GRID_HEIGHT] = {
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

static const uint16_t EXTRA_MINUTES_VARIANT_D[] = {
  static_cast<uint16_t>(LED_COUNT_GRID + 7),
  static_cast<uint16_t>(LED_COUNT_GRID + 9),
  static_cast<uint16_t>(LED_COUNT_GRID + 11),
  static_cast<uint16_t>(LED_COUNT_GRID + 13)
};

static const WordPosition WORDS_VARIANT_D[] = {
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

static const GridVariantData GRID_VARIANTS[] = {
  { GridVariant::Classic, "classic", "Standaard grid", LETTER_GRID_CLASSIC, countof(LETTER_GRID_CLASSIC), WORDS_CLASSIC, countof(WORDS_CLASSIC), EXTRA_MINUTES_CLASSIC, countof(EXTRA_MINUTES_CLASSIC) },
  { GridVariant::VariantB, "variant-b", "Variant B", LETTER_GRID_VARIANT_B, countof(LETTER_GRID_VARIANT_B), WORDS_VARIANT_B, countof(WORDS_VARIANT_B), EXTRA_MINUTES_VARIANT_B, countof(EXTRA_MINUTES_VARIANT_B) },
  { GridVariant::VariantC, "variant-c", "Variant C", LETTER_GRID_VARIANT_C, countof(LETTER_GRID_VARIANT_C), WORDS_VARIANT_C, countof(WORDS_VARIANT_C), EXTRA_MINUTES_VARIANT_C, countof(EXTRA_MINUTES_VARIANT_C) },
  { GridVariant::VariantD, "variant-d", "Variant D", LETTER_GRID_VARIANT_D, countof(LETTER_GRID_VARIANT_D), WORDS_VARIANT_D, countof(WORDS_VARIANT_D), EXTRA_MINUTES_VARIANT_D, countof(EXTRA_MINUTES_VARIANT_D) }
};

static const GridVariantData* activeVariant = &GRID_VARIANTS[0];

void applyActiveVariant(const GridVariantData* data) {
  activeVariant = data;
  LETTER_GRID = data->letterGrid;
  ACTIVE_WORDS = data->words;
  ACTIVE_WORD_COUNT = data->wordCount;
  EXTRA_MINUTE_LEDS = data->minuteLeds;
  EXTRA_MINUTE_LED_COUNT = data->minuteCount;
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

// Public state
const char* const* LETTER_GRID = LETTER_GRID_CLASSIC;
const WordPosition* ACTIVE_WORDS = WORDS_CLASSIC;
size_t ACTIVE_WORD_COUNT = countof(WORDS_CLASSIC);
const uint16_t* EXTRA_MINUTE_LEDS = EXTRA_MINUTES_CLASSIC;
size_t EXTRA_MINUTE_LED_COUNT = countof(EXTRA_MINUTES_CLASSIC);

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
  return setActiveGridVariant(static_cast<GridVariant>(id));
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

const GridVariantInfo* getGridVariantInfos(size_t& count) {
  static GridVariantInfo infos[countof(GRID_VARIANTS)];
  for (size_t i = 0; i < countof(GRID_VARIANTS); ++i) {
    infos[i].variant = GRID_VARIANTS[i].variant;
    infos[i].key = GRID_VARIANTS[i].key;
    infos[i].label = GRID_VARIANTS[i].label;
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
