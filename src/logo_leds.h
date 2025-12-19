#pragma once

#include <stdint.h>
#include <vector>

// Driver for the secondary logo LED strips.
// Allows storing and applying per-LED RGB colors while keeping brightness in sync
// with the main clock (including night mode dimming).
void initLogoLeds();
bool logoSetColor(uint16_t index, uint32_t rgb);
bool logoSetColors(const std::vector<uint32_t>& colors);
std::vector<uint32_t> logoGetColors();
void logoTick();  // Reapply brightness when night mode or brightness changes
uint8_t getLogoBrightness();
void setLogoBrightness(uint8_t level);
