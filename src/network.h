#pragma once
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "config.h"


extern WiFiServer telnetServer;
extern WiFiClient telnetClient;

void setupNetwork();
void setupOTA();
void setupTelnet();
void handleTelnet();
void resetWiFiSettings();