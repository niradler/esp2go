#include "api_server.h"
#include "config.h"
#include "modules/module_manager.h"
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);

AsyncWebServer& getWebServer() {
  return server;
}

void stopWebServer() {
  server.end();
}

void setupWebServer() {
  LOG_INFO("Setting up web server...");
  
  // Register all module APIs
  ModuleManager::getInstance().registerAllAPIs(server);
  
  // Start server
  server.begin();
  LOG_INFO("Web server started on port 80");
}
