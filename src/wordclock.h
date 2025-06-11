#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "log.h"
#include "led_controller.h"

// clockEnabled is defined externally in main.cpp
extern bool clockEnabled;

// Only declare the prototypes for setup/loop
void wordclock_setup();
void wordclock_loop();




