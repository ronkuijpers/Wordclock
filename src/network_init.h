#pragma once

#include <stdbool.h>

extern bool g_wifiHadCredentialsAtBoot;

void initNetwork();
void processNetwork();
bool isWiFiConnected();
void resetWiFiSettings();
