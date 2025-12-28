#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include "module_base.h"

class OTAModule : public ModuleBase {
public:
    const char* getName() const override { return "ota"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "Over-The-Air firmware updates"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    void getStatus(JsonObject& obj) const override;
    bool isReady() const override { return true; }
    
    bool startUpdate(const String& firmwarePath);
    void restart();
    
private:
    bool updateInProgress = false;
    int updateProgress = 0;
    String updateError = "";
};

#endif

