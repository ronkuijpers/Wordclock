#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "log.h"
#include "led_controller.h"

// clockEnabled wordt extern gedefinieerd in main.cpp
extern bool clockEnabled;

// Zet alleen nog de prototypes voor setup/loop
void wordclock_setup();
void wordclock_loop();




