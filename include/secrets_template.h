// secrets_template.h
#pragma once

#define OTA_PASSWORD  "your_ota_password"
#define AP_PASSWORD "your_ap_password"

#define VERSION_URL "version_url"

#define ADMIN_USER "admin_user"
#define ADMIN_PASS "admin_pass"
#define ADMIN_REALM "admin_realm"

// Default UI password (user is fixed as "user"). User must change on first login.
#ifndef UI_DEFAULT_PASS
#define UI_DEFAULT_PASS "changeme"
#endif

// MQTT broker configuration
#define MQTT_HOST   "192.168.1.10"
#define MQTT_PORT   1883
#define MQTT_USER   "mqtt_user"
#define MQTT_PASS   "mqtt_pass"
// Home Assistant discovery prefix and base topic for this device
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#define MQTT_BASE_TOPIC       "wordclock"
