#include "ota_module.h"
#include "../config.h"
#include <Update.h>
#include <SD.h>

bool OTAModule::init() {
    logInfo("Initializing...");
    
    updateInProgress = false;
    updateProgress = 0;
    updateError = "";
    
    logInfo("Initialized successfully");
    return true;
}

void OTAModule::update() {
}

void OTAModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["update_in_progress"] = updateInProgress;
    obj["progress"] = updateProgress;
    if (updateError.length() > 0) {
        obj["error"] = updateError;
    }
}

bool OTAModule::startUpdate(const String& firmwarePath) {
    if (updateInProgress) {
        logWarn("Update already in progress");
        return false;
    }
    
    if (!SD.exists(firmwarePath)) {
        updateError = "Firmware file not found";
        logError("%s: %s", updateError.c_str(), firmwarePath.c_str());
        return false;
    }
    
    File firmware = SD.open(firmwarePath);
    if (!firmware) {
        updateError = "Failed to open firmware file";
        logError("%s", updateError.c_str());
        return false;
    }
    
    size_t fileSize = firmware.size();
    logInfo("Starting OTA update (size: %d bytes)", fileSize);
    
    updateInProgress = true;
    updateProgress = 0;
    updateError = "";
    
    if (!Update.begin(fileSize)) {
        updateError = "Not enough space for update";
        logError("%s", updateError.c_str());
        firmware.close();
        updateInProgress = false;
        return false;
    }
    
    size_t written = Update.writeStream(firmware);
    firmware.close();
    
    if (written != fileSize) {
        updateError = "Update write failed";
        logError("%s (written: %d, expected: %d)", updateError.c_str(), written, fileSize);
        Update.abort();
        updateInProgress = false;
        return false;
    }
    
    if (!Update.end(true)) {
        updateError = "Update end failed";
        logError("%s", updateError.c_str());
        updateInProgress = false;
        return false;
    }
    
    logInfo("OTA update completed successfully");
    updateInProgress = false;
    updateProgress = 100;
    
    return true;
}

void OTAModule::restart() {
    logInfo("Restarting after OTA update...");
    delay(1000);
    ESP.restart();
}

void OTAModule::registerAPI(AsyncWebServer& server) {
    // OTA status
    server.on("/_api/ota/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getStatus(obj);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // OTA update
    server.on("/_api/ota/update", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (updateInProgress) {
                request->send(400, "application/json", "{\"error\":\"Update in progress\"}");
                return;
            }
            
            if (!request->hasParam("path")) {
                request->send(400, "application/json", "{\"error\":\"Missing path parameter\"}");
                return;
            }
            
            String path = request->getParam("path")->value();
            if (path.length() == 0) {
                path = PATH_FIRMWARE_DEFAULT;
            }
            
            logInfo("OTA update requested for: %s", path.c_str());
            
            if (startUpdate(path)) {
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Update completed, restarting...\"}");
                delay(500);
                restart();
            } else {
                JsonDocument doc;
                doc["error"] = "Update failed";
                doc["message"] = updateError;
                
                String response;
                serializeJson(doc, response);
                request->send(500, "application/json", response);
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                logInfo("OTA upload started: %s", filename.c_str());
                
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    logError("OTA begin failed");
                    return;
                }
            }
            
            if (len) {
                if (Update.write(data, len) != len) {
                    logError("OTA write failed");
                    return;
                }
                updateProgress = (Update.progress() * 100) / Update.size();
            }
            
            if (final) {
                if (Update.end(true)) {
                    logInfo("OTA upload completed: %u bytes", index + len);
                    updateProgress = 100;
                } else {
                    logError("OTA end failed");
                }
            }
        }
    );
}

