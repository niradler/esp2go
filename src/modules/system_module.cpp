#include "system_module.h"
#include "../config.h"
#include <esp_system.h>

bool SystemModule::init() {
    logInfo("Initializing...");
    startTime = millis();
    logInfo("Initialized successfully");
    return true;
}

void SystemModule::update() {
    // Lazy by default - system info only when API is called
}

void SystemModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["free_heap"] = getFreeHeap();
    obj["uptime"] = getUptime();
    obj["wifi_connected"] = WiFi.status() == WL_CONNECTED;
}

void SystemModule::getSystemInfo(JsonObject& info) {
    info["chip"] = ESP.getChipModel();
    info["revision"] = ESP.getChipRevision();
    info["cpu_freq"] = ESP.getCpuFreqMHz();
    info["free_heap"] = ESP.getFreeHeap();
    info["min_free_heap"] = ESP.getMinFreeHeap();
    info["max_alloc_heap"] = ESP.getMaxAllocHeap();
    info["flash_size"] = ESP.getFlashChipSize();
    info["uptime"] = getUptime();
    info["sdk_version"] = esp_get_idf_version();
}

void SystemModule::getWiFiStatus(JsonObject& status) {
    status["connected"] = WiFi.status() == WL_CONNECTED;
    status["ssid"] = WiFi.SSID();
    status["ip"] = WiFi.localIP().toString();
    status["rssi"] = WiFi.RSSI();
    status["mac"] = WiFi.macAddress();
    status["mode"] = WiFi.getMode() == WIFI_STA ? "STA" : WiFi.getMode() == WIFI_AP ? "AP" : "AP_STA";
}

void SystemModule::restart() {
    logInfo("Restarting system...");
    delay(100);
    ESP.restart();
}

uint32_t SystemModule::getFreeHeap() const {
    return ESP.getFreeHeap();
}

uint32_t SystemModule::getMinFreeHeap() const {
    return ESP.getMinFreeHeap();
}

uint32_t SystemModule::getMaxAllocHeap() const {
    return ESP.getMaxAllocHeap();
}

float SystemModule::getCPUTemperature() const {
    return temperatureRead();
}

uint32_t SystemModule::getUptime() const {
    return (millis() - startTime) / 1000;
}

void SystemModule::registerAPI(AsyncWebServer& server) {
    // System info
    server.on("/_api/system/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject info = doc.to<JsonObject>();
        getSystemInfo(info);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // WiFi status
    server.on("/_api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject status = doc.to<JsonObject>();
        getWiFiStatus(status);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Performance metrics
    server.on("/_api/system/performance", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["free_heap"] = getFreeHeap();
        doc["min_free_heap"] = getMinFreeHeap();
        doc["max_alloc_heap"] = getMaxAllocHeap();
        doc["cpu_temp"] = getCPUTemperature();
        doc["uptime"] = getUptime();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Restart
    server.on("/_api/system/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"status\":\"restarting\"}");
        restart();
    });
}

