#pragma once

// Afmetingen van het lettergrid
const int GRID_WIDTH = 11;
const int GRID_HEIGHT = 10;
const int LED_COUNT_GRID = GRID_WIDTH * GRID_HEIGHT;
const int LED_COUNT_EXTRA = 4;  // minuten-leds
const int LED_COUNT_TOTAL = LED_COUNT_GRID + LED_COUNT_EXTRA;

// De layout van het grid (alleen voor referentie of debugging)
const char* LETTER_GRID[GRID_HEIGHT] = {
  "HETBISWYBRC", 
  "RTIENMMUHLC",
  "VIJFCWKWART",
  "OVERXTTXLVB",
  "QKEVOORTFIG",
  "DRIEKBZEVEN",
  "VTTIENELNRC",
  "TWAALFSFRSF",
  "EENEGENACHT",
  "XEVIJFJXUUR"
};

// De indexen van de 4 extra minuut-leds (na het hoofdgrid)
const int EXTRA_MINUTE_LEDS[4] = {
    LED_COUNT_GRID + 0,
    LED_COUNT_GRID + 1,
    LED_COUNT_GRID + 2,
    LED_COUNT_GRID + 3
  };