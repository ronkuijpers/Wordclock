#include "log.h"

LogLevel LOG_LEVEL = DEFAULT_LOG_LEVEL;

String logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;

void log(String msg, int level) {
  if (level > LOG_LEVEL) return;

  // if (telnetClient && telnetClient.connected()) {
  //   telnetClient.print(msg);
  // }

  Serial.print(msg);

  if (level >= LOG_LEVEL_INFO) {
    logBuffer[logIndex] = msg;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
  }
}

void logln(String msg, int level) {
  log(msg + "\n", level);
}

void setLogLevel(LogLevel level) {
  LOG_LEVEL = level;
}
