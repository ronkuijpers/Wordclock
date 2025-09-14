#pragma once

#include <WebServer.h>
#include "web_routes.h"
#include "log.h"

// Initialiseer webserver en routes
inline void initWebServer(WebServer& server) {
    setupWebRoutes();
    server.begin();
    logInfo("🟢 Webserver gestart en routes geactiveerd");
}
