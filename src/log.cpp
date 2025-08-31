#include "log.h"
#include <Preferences.h>

LogLevel LOG_LEVEL = DEFAULT_LOG_LEVEL;

String logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;

void log(String msg, int level) {
  // Filter: only log messages at or above current threshold
  if (level < LOG_LEVEL) return;

  // if (telnetClient && telnetClient.connected()) {
  //   telnetClient.print(msg);
  // }

  Serial.print(msg);

  // Store in ring buffer any message that passes the filter
  logBuffer[logIndex] = msg;
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
}

void logln(String msg, int level) {
  log(msg + "\n", level);
}

void setLogLevel(LogLevel level) {
  LOG_LEVEL = level;
  // Persist new level
  Preferences prefs;
  prefs.begin("log", false);
  prefs.putUChar("level", (uint8_t)level);
  prefs.end();
}

void initLogSettings() {
  // Load persisted level if available
  Preferences prefs;
  prefs.begin("log", true);
  uint8_t lvl = prefs.getUChar("level", (uint8_t)DEFAULT_LOG_LEVEL);
  prefs.end();
  if (lvl <= LOG_LEVEL_ERROR) {
    LOG_LEVEL = (LogLevel)lvl;
  } else {
    LOG_LEVEL = DEFAULT_LOG_LEVEL;
  }
}
