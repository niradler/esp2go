#include "microphone_module.h"
#include "../config.h"
#include "../hardware.h"

bool MicrophoneModule::init() {
    logInfo("Initializing...");
    
    isRecording = false;
    currentLevel = 0;
    recordingStartTime = 0;
    
    logInfo("Initialized successfully");
    return true;
}

void MicrophoneModule::update() {
    // Only update when recording (lazy by default)
    if (isRecording) {
        // Process recording data
        processRecording();
        
        uint32_t elapsed = millis() - recordingStartTime;
        if (elapsed > maxRecordingDuration) {
            stopRecording();
            logWarn("Recording stopped: max duration reached");
        }
    }
}

void MicrophoneModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["level"] = currentLevel;
    obj["recording"] = isRecording;
    if (isRecording) {
        obj["recording_duration"] = (millis() - recordingStartTime) / 1000;
    }
}

int MicrophoneModule::getLevel() {
    // Read microphone level on-demand (lazy)
    currentLevel = readMicrophoneLevel();
    return currentLevel;
}

bool MicrophoneModule::startRecording() {
    if (isRecording) {
        logWarn("Already recording");
        return false;
    }
    
    isRecording = true;
    recordingStartTime = millis();
    logInfo("Recording started");
    return true;
}

bool MicrophoneModule::stopRecording() {
    if (!isRecording) {
        logWarn("Not recording");
        return false;
    }
    
    isRecording = false;
    uint32_t duration = (millis() - recordingStartTime) / 1000;
    logInfo("Recording stopped (duration: %d seconds)", duration);
    return true;
}

bool MicrophoneModule::getRecordingStatus() {
    return isRecording;
}

void MicrophoneModule::registerAPI(AsyncWebServer& server) {
    // Get microphone level
    server.on("/_api/microphone/level", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["level"] = getLevel();
        doc["timestamp"] = millis();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Start recording
    server.on("/_api/microphone/record/start", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (startRecording()) {
                request->send(200, "application/json", "{\"status\":\"recording\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Already recording\"}");
            }
        }
    );
    
    // Stop recording
    server.on("/_api/microphone/record/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (stopRecording()) {
            uint32_t duration = (millis() - recordingStartTime) / 1000;
            JsonDocument doc;
            doc["status"] = "stopped";
            doc["duration"] = duration;
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json", "{\"error\":\"Not recording\"}");
        }
    });
    
    // Get recording status
    server.on("/_api/microphone/record/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["recording"] = getRecordingStatus();
        if (isRecording) {
            doc["duration"] = (millis() - recordingStartTime) / 1000;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}

