#include "led_module.h"
#include "../config.h"
#include "../hardware.h"

bool LEDModule::init() {
    logInfo("Initializing...");
    
    // Initialize LED hardware
    currentR = 0;
    currentG = 0;
    currentB = 0;
    brightness = 255;
    isOn = false;
    
    // Turn off LED
    setLED(0, 0, 0);
    
    initialized = true;
    logInfo("Initialized successfully");
    return true;
}

void LEDModule::update() {
    // Handle blink pattern if active
    if (blinkActive && millis() - lastBlinkTime >= blinkInterval) {
        lastBlinkTime = millis();
        blinkState = !blinkState;
        
        if (blinkState) {
            setLED(currentR, currentG, currentB);
        } else {
            setLED(0, 0, 0);
        }
        
        // Check if blink should stop
        if (blinkCount > 0) {
            blinkCurrent++;
            if (blinkCurrent >= blinkCount * 2) {
                stopBlink();
            }
        }
    }
}

void LEDModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["is_on"] = isOn;
    obj["r"] = currentR;
    obj["g"] = currentG;
    obj["b"] = currentB;
    obj["brightness"] = brightness;
    obj["blinking"] = blinkActive;
}

void LEDModule::setColor(uint8_t r, uint8_t g, uint8_t b) {
    currentR = r;
    currentG = g;
    currentB = b;
    isOn = (r > 0 || g > 0 || b > 0);
    
    // Apply brightness
    uint8_t br = (r * brightness) / 255;
    uint8_t bg = (g * brightness) / 255;
    uint8_t bb = (b * brightness) / 255;
    
    setLED(br, bg, bb);
}

void LEDModule::off() {
    currentR = 0;
    currentG = 0;
    currentB = 0;
    isOn = false;
    stopBlink();
    setLED(0, 0, 0);
}

void LEDModule::setBrightness(uint8_t value) {
    brightness = value;
    // Reapply current color with new brightness
    if (isOn) {
        setColor(currentR, currentG, currentB);
    }
}

void LEDModule::startBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t interval, int count) {
    currentR = r;
    currentG = g;
    currentB = b;
    blinkInterval = interval;
    blinkCount = count;
    blinkCurrent = 0;
    blinkActive = true;
    blinkState = true;
    lastBlinkTime = millis();
    isOn = true;
}

void LEDModule::stopBlink() {
    blinkActive = false;
    blinkState = false;
}

void LEDModule::registerAPI(AsyncWebServer& server) {
    // Set LED color
    server.on("/_api/led/set", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            if (!doc["r"].is<int>() || !doc["g"].is<int>() || !doc["b"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Missing r, g, b values\"}");
                return;
            }
            
            uint8_t r = doc["r"].as<int>();
            uint8_t g = doc["g"].as<int>();
            uint8_t b = doc["b"].as<int>();
            
            setColor(r, g, b);
            
            JsonDocument resp;
            resp["status"] = "ok";
            resp["r"] = r;
            resp["g"] = g;
            resp["b"] = b;
            
            String response;
            serializeJson(resp, response);
            request->send(200, "application/json", response);
        }
    );
    
    // Turn off LED
    server.on("/_api/led/off", HTTP_POST, [this](AsyncWebServerRequest *request) {
        off();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // Set brightness
    server.on("/_api/led/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error || !doc["value"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid brightness value\"}");
                return;
            }
            
            uint8_t value = doc["value"].as<int>();
            if (value > 255) value = 255;
            
            setBrightness(value);
            request->send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(value) + "}");
        }
    );
    
    // Start blink
    server.on("/_api/led/blink", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            uint8_t r = doc["r"].is<int>() ? doc["r"].as<int>() : 255;
            uint8_t g = doc["g"].is<int>() ? doc["g"].as<int>() : 0;
            uint8_t b = doc["b"].is<int>() ? doc["b"].as<int>() : 0;
            uint32_t interval = doc["interval"].is<int>() ? doc["interval"].as<int>() : 500;
            int count = doc["count"].is<int>() ? doc["count"].as<int>() : 0;
            
            startBlink(r, g, b, interval, count);
            request->send(200, "application/json", "{\"status\":\"blinking\"}");
        }
    );
    
    // Get status
    server.on("/_api/led/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getStatus(obj);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}

