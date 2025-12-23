#pragma once
#include <Arduino.h>
// #include "network.h"  // For access to telnetClient
#include "config.h"

enum LogLevel {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR
};
extern LogLevel LOG_LEVEL;

// Basic log function
void log(String msg, int level = LOG_LEVEL_INFO);
void logln(String msg, int level = LOG_LEVEL_INFO);

// Convenience functions
#define logDebug(msg) logln(msg, LOG_LEVEL_DEBUG)
#define logInfo(msg)  logln(msg, LOG_LEVEL_INFO)
#define logWarn(msg)  logln(msg, LOG_LEVEL_WARN)
#define logError(msg) logln(msg, LOG_LEVEL_ERROR)

void setLogLevel(LogLevel level);
void initLogSettings();
void logEnableFileSink();
void logFlushFile();
String logLatestFilePath();
void logRewriteUnsynced();
