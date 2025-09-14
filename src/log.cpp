#include "log.h"
#include <Preferences.h>
#include <time.h>
#include <stdlib.h>

LogLevel LOG_LEVEL = DEFAULT_LOG_LEVEL;

String logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;

static inline const char* levelToTag(int level) {
  switch (level) {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO:  return "INFO";
    case LOG_LEVEL_WARN:  return "WARN";
    case LOG_LEVEL_ERROR: return "ERROR";
    default:              return "INFO";
  }
}

static String makeLogPrefix(int level) {
  // Prefer localtime_r with TZ applied; fall back to uptime if RTC not set yet
  time_t now = time(nullptr);
  // Consider time unsynced if before 2022-01-01
  if (now < 1640995200) {
    unsigned long nowMs = millis();
    char out[64];
    snprintf(out, sizeof(out), "[uptime %lu.%03lus][%s] ", nowMs/1000UL, nowMs%1000UL, levelToTag(level));
    return String(out);
  }

  struct tm lt = {};
  localtime_r(&now, &lt);
  char datebuf[32];
  char tzbuf[8];
  strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M:%S", &lt);
  strftime(tzbuf, sizeof(tzbuf), "%Z", &lt);
  unsigned long ms = millis() % 1000UL;
  char out[80];
  // Format: [YYYY-MM-DD HH:MM:SS.mmm TZ][LEVEL]
  snprintf(out, sizeof(out), "[%s.%03lu %s][%s] ", datebuf, ms, tzbuf, levelToTag(level));
  return String(out);
}

void log(String msg, int level) {
  // Filter: only log messages at or above current threshold
  if (level < LOG_LEVEL) return;

  // if (telnetClient && telnetClient.connected()) {
  //   telnetClient.print(msg);
  // }

  String line = makeLogPrefix(level) + msg;
  Serial.print(line);

  // Store in ring buffer any message that passes the filter
  logBuffer[logIndex] = line;
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

  // Apply timezone as early as possible so logs use local time
  setenv("TZ", TZ_INFO, 1);
  tzset();
}
