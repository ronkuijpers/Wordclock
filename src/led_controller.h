#pragma once
#include <FastLED.h>
#include <vector>

void initLeds();
void showLeds(const std::vector<int>& indices, CRGB color = CRGB::White);
