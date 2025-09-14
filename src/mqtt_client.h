#pragma once

#include <Arduino.h>

void mqtt_begin();
void mqtt_loop();
void mqtt_publish_state(bool force = false);

