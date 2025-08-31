#pragma once

#define FIRMWARE_VERSION "0.3"
#define UI_VERSION "0.4"

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
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#define LOG_BUFFER_SIZE 150  // for example
