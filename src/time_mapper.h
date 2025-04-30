#pragma once
#include <vector>
#include <time.h>

// Declaraties van je mapper-helpers
std::vector<uint16_t> get_leds_for_word(const char* word);
std::vector<uint16_t> merge_leds(std::initializer_list<std::vector<uint16_t>> lists);
std::vector<uint16_t> get_led_indices_for_time(struct tm* timeinfo);
