#ifndef STORAGE_MODULE_H
#define STORAGE_MODULE_H

#include "module_base.h"

class StorageModule : public ModuleBase {
public:
    const char* getName() const override { return "storage"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "SD card file management"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    void getStatus(JsonObject& obj) const override;
    bool isReady() const override { return true; }
    
private:
    void updateStats();
    
    uint64_t totalSpace = 0;
    uint64_t usedSpace = 0;
    uint64_t freeSpace = 0;
    uint32_t lastStatsUpdate = 0;
};

#endif

