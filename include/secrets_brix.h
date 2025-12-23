// secrets_brix.h
#pragma once

#define OTA_PASSWORD  "wordclockota"
#define AP_PASSWORD "wordclockAP"

#define VERSION_URL "https://raw.githubusercontent.com/ronkuijpers/Wordclock/main/firmware.json"

#define ADMIN_USER "admin"
#define ADMIN_PASS "IqvY50lh7u8yuNIsaeaaYwJ9Zl5T4G3tLkWr5h625c8fQreRky"
#define ADMIN_REALM "Wordclock Admin"

// Default UI password (user is fixed as "user"). User must change on first login.
#ifndef UI_DEFAULT_PASS
#define UI_DEFAULT_PASS "changeme"
#endif
