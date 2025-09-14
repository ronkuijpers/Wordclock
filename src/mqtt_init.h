#pragma once

#include "mqtt_client.h"
#include "log.h"

// Initialize MQTT
// This function starts the MQTT client and ensures connection to the broker.
inline void initMqtt() {
    mqtt_begin();
    logInfo("🟢 MQTT started");
}

// MQTT event loop
// This function processes incoming and outgoing MQTT messages.
inline void mqttEventLoop() {
    mqtt_loop();
}
