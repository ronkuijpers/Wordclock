// fs_compat.h
#pragma once

// For 0.x releases we unify on SPIFFS. If/when migrating to LittleFS,
// flip these includes and the FS_IMPL define.
#include <SPIFFS.h>

#define FS_IMPL SPIFFS

