// time_mapper.cpp
#include <vector>
#include <string>
#include <time.h>
#include "grid_layout.h"
#include "log.h"
#include "wordposition.h"
#include "time_mapper.h"


// Helper: maps a word string to its LED indices
std::vector<uint16_t> get_leds_for_word(const char* word) {
  std::vector<uint16_t> result;
  for (int i = 0; i < WORDS_COUNT; ++i) {
    const auto& w = WORDS[i];
    if (strcmp(w.word, word) == 0) {
      for (int i = 0; i < 10 && w.indices[i] != 0; ++i) {
        result.push_back(w.indices[i]);
      }
      break;
    }
  }
  return result;
}

// Helper: merges multiple LED vectors
std::vector<uint16_t> merge_leds(std::initializer_list<std::vector<uint16_t>> lists) {
  std::vector<uint16_t> result;
  for (const auto& list : lists) {
    result.insert(result.end(), list.begin(), list.end());
  }
  return result;
}

std::vector<uint16_t> get_led_indices_for_time(struct tm* timeinfo) {
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;

  // Always round down to the lower 5-minute interval
  int rounded_minute = (minute / 5) * 5;
  int extra_minutes = minute % 5;

  if (rounded_minute == 60) {
    rounded_minute = 0;
    hour = (hour + 1) % 24;
  }

  int hour12 = hour % 12;
  if (rounded_minute >= 20) hour12 = (hour12 + 1) % 12; // After 'over' or 'half' we look at next hour

  static const char* HOURS[] = {
    "TWAALF", "EEN", "TWEE", "DRIE", "VIER", "VIJF", "ZES",
    "ZEVEN", "ACHT", "NEGEN", "TIEN", "ELF"
  };

  std::vector<uint16_t> leds = get_leds_for_word("HET IS");

  switch (rounded_minute) {
    case 0:
      leds = merge_leds({leds, get_leds_for_word(HOURS[hour12]), get_leds_for_word("UUR")});
      break;
    case 5:
      leds = merge_leds({leds, get_leds_for_word("VIJF OVER"), get_leds_for_word(HOURS[hour12])});
      break;
    case 10:
      leds = merge_leds({leds, get_leds_for_word("TIEN OVER"), get_leds_for_word(HOURS[hour12])});
      break;
    case 15:
      leds = merge_leds({leds, get_leds_for_word("KWART OVER"), get_leds_for_word(HOURS[hour12])});
      break;
    case 20:
      leds = merge_leds({leds, get_leds_for_word("TIEN VOOR"), get_leds_for_word("HALF"), get_leds_for_word(HOURS[hour12])});
      break;
    case 25:
      leds = merge_leds({leds, get_leds_for_word("VIJF VOOR"), get_leds_for_word("HALF"), get_leds_for_word(HOURS[hour12])});
      break;
    case 30:
      leds = merge_leds({leds, get_leds_for_word("HALF"), get_leds_for_word(HOURS[hour12])});
      break;
    case 35:
      leds = merge_leds({leds, get_leds_for_word("VIJF OVER"), get_leds_for_word("HALF"), get_leds_for_word(HOURS[hour12])});
      break;
    case 40:
      leds = merge_leds({leds, get_leds_for_word("TIEN OVER"), get_leds_for_word("HALF"), get_leds_for_word(HOURS[hour12])});
      break;
    case 45:
      leds = merge_leds({leds, get_leds_for_word("KWART VOOR"), get_leds_for_word(HOURS[hour12])});
      break;
    case 50:
      leds = merge_leds({leds, get_leds_for_word("TIEN VOOR"), get_leds_for_word(HOURS[hour12])});
      break;
    case 55:
      leds = merge_leds({leds, get_leds_for_word("VIJF VOOR"), get_leds_for_word(HOURS[hour12])});
      break;
  }

  // Add extra minute LEDs if needed
  for (int i = 0; i < extra_minutes && i < 4; ++i) {
    leds.push_back(EXTRA_MINUTE_LEDS[i]);
  }

  return leds;
}