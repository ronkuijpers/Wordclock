#pragma once
#include <Arduino.h>
#include "network.h"  // Voor toegang tot telnetClient
#include "config.h"

// Basis logfunctie
void log(String msg, int level = LOG_LEVEL_INFO);
void logln(String msg, int level = LOG_LEVEL_INFO);

// Convenience functies
#define logDebug(msg) logln(msg, LOG_LEVEL_DEBUG)
#define logInfo(msg)  logln(msg, LOG_LEVEL_INFO)
#define logError(msg) logln(msg, LOG_LEVEL_ERROR)
