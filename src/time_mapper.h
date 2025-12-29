#pragma once
#include <vector>
#include <time.h>

struct WordSegment {
  const char* key;
  std::vector<uint16_t> leds;
};

// Declarations of mapper helpers
std::vector<uint16_t> get_leds_for_word(const char* word);
std::vector<uint16_t> merge_leds(std::initializer_list<std::vector<uint16_t>> lists);
std::vector<uint16_t> get_led_indices_for_time(struct tm* timeinfo);
// Returns the word segments (without extra minute LEDs) for the given time
std::vector<WordSegment> get_word_segments_with_keys(struct tm* timeinfo);
std::vector<std::vector<uint16_t>> get_word_segments_for_time(struct tm* timeinfo);
