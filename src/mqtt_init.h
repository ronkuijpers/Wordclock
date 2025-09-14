#pragma once

#include "mqtt_client.h"
#include "log.h"

// Initialiseer MQTT
inline void initMqtt() {
    mqtt_begin();
    logInfo("🟢 MQTT gestart");
}

// MQTT event loop
inline void mqttEventLoop() {
    mqtt_loop();
}
