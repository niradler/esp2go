#ifndef SYSTEM_MODULE_H
#define SYSTEM_MODULE_H

#include "module_base.h"
#include <WiFi.h>

class SystemModule : public ModuleBase {
public:
    const char* getName() const override { return "system"; }
    const char* getDescription() const override { return "System Information and Control"; }
    
    bool init() override;
    void registerAPI(AsyncWebServer& server) override;
    void update() override;
    bool isReady() const override { return true; }  // Always ready
    void getStatus(JsonObject& obj) const override;
    
    // System Functions
    void getSystemInfo(JsonObject& info);
    void getWiFiStatus(JsonObject& status);
    void restart();
    void factoryReset();
    
    // Performance monitoring
    uint32_t getFreeHeap() const;
    uint32_t getMinFreeHeap() const;
    uint32_t getMaxAllocHeap() const;
    float getCPUTemperature() const;
    uint32_t getUptime() const;
    
private:
    uint32_t startTime = 0;
    uint32_t lastHeapCheck = 0;
};

#endif

