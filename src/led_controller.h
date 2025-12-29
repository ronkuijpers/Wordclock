#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#ifndef PIO_UNIT_TESTING
#include <Adafruit_NeoPixel.h>
#endif
#include <vector>

// Export the function prototypes:
void initLeds();
void showLeds(const std::vector<uint16_t> &ledIndices);
void showLedsCombined(const std::vector<uint16_t> &ledIndices, bool includeLogo);
void blinkAllLeds(uint8_t blinks = 3, uint16_t onMs = 200, uint16_t offMs = 200);

#ifdef PIO_UNIT_TESTING
const std::vector<uint16_t>& test_getLastShownLeds();
void test_clearLastShownLeds();
#endif

#endif // LED_CONTROLLER_H
