// Log buffer and default log level
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#define LOG_BUFFER_SIZE 150
#pragma once

#define FIRMWARE_VERSION "0.6"
#define UI_VERSION "0.6"

#define NUM_LEDS 161
#define DATA_PIN 4
#define DEFAULT_BRIGHTNESS 5

#define CLOCK_NAME "Wordclock"
#define AP_NAME "Wordclock_AP"
#define OTA_PORT 3232

// Time
#define TZ_INFO "CET-1CEST,M3.5.0/2,M10.5.0/3"
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"

// Logging

#define SERIAL_BAUDRATE 115200
#define WIFI_CONFIG_PORTAL_TIMEOUT 180 // seconds
#define WIFI_CONNECT_MAX_RETRIES 20
#define WIFI_CONNECT_RETRY_DELAY_MS 500
#define MDNS_HOSTNAME "wordclock"
#define MDNS_START_DELAY_MS 1000

#define OTA_UPDATE_COMPLETE_DELAY_MS 1000
#define EEPROM_WRITE_DELAY_MS 500

#define DAILY_FIRMWARE_CHECK_HOUR 2
#define DAILY_FIRMWARE_CHECK_MINUTE 0
#define DAILY_FIRMWARE_CHECK_INTERVAL_SEC 3600

constexpr unsigned long SWEEP_STEP_MS = 20;
constexpr unsigned long IP_STEP_MS = 1000;
constexpr uint16_t IP_ZERO_O_LED_INDEX = 56;

