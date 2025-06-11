#pragma once
#include <Arduino.h>
// #include "network.h"  // For access to telnetClient
#include "config.h"

// Basic log function
void log(String msg, int level = LOG_LEVEL_INFO);
void logln(String msg, int level = LOG_LEVEL_INFO);

// Convenience functions
#define logDebug(msg) logln(msg, LOG_LEVEL_DEBUG)
#define logInfo(msg)  logln(msg, LOG_LEVEL_INFO)
#define logError(msg) logln(msg, LOG_LEVEL_ERROR)
