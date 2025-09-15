// secrets_template.h
#pragma once

#define OTA_PASSWORD  "your_ota_password"
#define AP_PASSWORD "your_ap_password"

#define VERSION_URL "version_url"

#define ADMIN_USER "admin_user"
#define ADMIN_PASS "admin_pass"
#define ADMIN_REALM "admin_realm"

// Default UI password (user is fixed as "user"). User must change on first login.
#ifndef UI_DEFAULT_PASS
#define UI_DEFAULT_PASS "changeme"
#endif
