#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include "module_base.h"

class WebServerModule : public ModuleBase {
public:
    const char* getName() const override { return "web_server"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "Serves static files from SD card"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    bool isReady() const override { return true; }
    
private:
    void serveFallbackIndex(AsyncWebServerRequest *request);
    String getContentType(const String& path);
};

#endif

