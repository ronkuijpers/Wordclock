#pragma once

#include <WebServer.h>
#include "web_routes.h"
#include "log.h"

// Initialize webserver and routes
// This function registers all webserver endpoints and starts the webserver.
// Ensures the UI and API are accessible over the network.
inline void initWebServer(WebServer& server) {
    setupWebRoutes();
    server.begin();
    logInfo("🟢 Webserver started and routes activated");
}
