#include "button_module.h"
#include "../config.h"
#include "../hardware.h"

bool ButtonModule::init() {
    logInfo("Initializing...");
    
    buttonState = false;
    lastButtonState = false;
    lastDebounceTime = 0;
    debounceDelay = 50;
    
    logInfo("Initialized successfully");
    return true;
}

void ButtonModule::update() {
    // Lazy by default - no continuous polling
}

void ButtonModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["pressed"] = buttonState;
}

bool ButtonModule::isPressed() {
    // Read button on-demand (lazy)
    bool reading = readButton();
    
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            
            if (buttonState) {
                logInfo("Button pressed");
            } else {
                logInfo("Button released");
            }
        }
    }
    
    lastButtonState = reading;
    return buttonState;
}

void ButtonModule::registerAPI(AsyncWebServer& server) {
    // Get button status
    server.on("/_api/button/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["pressed"] = isPressed();
        doc["timestamp"] = millis();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}

