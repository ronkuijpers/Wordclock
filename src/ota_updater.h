// Public OTA functions (implementation in ota_updater.cpp)
#pragma once

void syncFilesFromManifest();
void syncUiFilesFromConfiguredVersion();
void checkForFirmwareUpdate();
