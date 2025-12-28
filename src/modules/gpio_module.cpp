#include "gpio_module.h"

bool GPIOModule::init() {
    logInfo("Initializing GPIO module...");
    
    // Initialize pin tracking
    pinModes.clear();
    
    initialized = true;
    logInfo("GPIO module initialized with %d available pins", sizeof(availablePins));
    return true;
}

bool GPIOModule::isPinAvailable(uint8_t pin) {
    // Check if pin is in reserved list
    for (uint8_t reserved : reservedPins) {
        if (pin == reserved) return false;
    }
    
    // Check if pin is in available list
    for (uint8_t available : availablePins) {
        if (pin == available) return true;
    }
    
    return false;
}

bool GPIOModule::isPinADC(uint8_t pin) {
    for (uint8_t adc : adcPins) {
        if (pin == adc) return true;
    }
    return false;
}

bool GPIOModule::isPinPWM(uint8_t pin) {
    for (uint8_t pwm : pwmPins) {
        if (pin == pwm) return true;
    }
    return false;
}

bool GPIOModule::pinMode(uint8_t pin, uint8_t mode) {
    if (!isPinAvailable(pin)) {
        logWarn("Pin %d is not available or reserved", pin);
        return false;
    }
    
    ::pinMode(pin, mode);
    pinModes[pin] = mode;
    logInfo("Set pin %d mode to %d", pin, mode);
    return true;
}

bool GPIOModule::digitalWrite(uint8_t pin, uint8_t value) {
    if (!isPinAvailable(pin)) {
        logWarn("Pin %d is not available", pin);
        return false;
    }
    
    ::digitalWrite(pin, value);
    return true;
}

int GPIOModule::digitalRead(uint8_t pin) {
    if (!isPinAvailable(pin)) {
        logWarn("Pin %d is not available", pin);
        return -1;
    }
    
    return ::digitalRead(pin);
}

int GPIOModule::analogRead(uint8_t pin) {
    if (!isPinADC(pin)) {
        logWarn("Pin %d does not support ADC", pin);
        return -1;
    }
    
    return ::analogRead(pin);
}

bool GPIOModule::analogWrite(uint8_t pin, uint16_t value) {
    if (!isPinPWM(pin)) {
        logWarn("Pin %d does not support PWM", pin);
        return false;
    }
    
    analogWrite(pin, value);
    return true;
}

void GPIOModule::getAvailablePins(JsonArray& array) {
    for (uint8_t pin : availablePins) {
        array.add(pin);
    }
}

void GPIOModule::getReservedPins(JsonArray& array) {
    for (uint8_t pin : reservedPins) {
        array.add(pin);
    }
}

void GPIOModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["available_pins"] = sizeof(availablePins);
    obj["reserved_pins"] = sizeof(reservedPins);
    obj["configured_pins"] = pinModes.size();
}

void GPIOModule::registerAPI(AsyncWebServer& server) {
    // Get available pins
    server.on("/_api/gpio/pins", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonArray available = doc["available"].to<JsonArray>();
        JsonArray reserved = doc["reserved"].to<JsonArray>();
        
        getAvailablePins(available);
        getReservedPins(reserved);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Set pin mode
    server.on("/_api/gpio/mode", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index != 0) return;
        
        String body((char*)data, len);
        JsonDocument doc;
        if (deserializeJson(doc, body) != DeserializationError::Ok) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (!doc["pin"].is<uint8_t>() || !doc["mode"].is<uint8_t>()) {
            request->send(400, "application/json", "{\"error\":\"Missing pin or mode\"}");
            return;
        }
        
        uint8_t pin = doc["pin"];
        uint8_t mode = doc["mode"];
        
        if (pinMode(pin, mode)) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid pin or mode\"}");
        }
    });
    
    // Digital write
    server.on("/_api/gpio/write", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index != 0) return;
        
        String body((char*)data, len);
        JsonDocument doc;
        if (deserializeJson(doc, body) != DeserializationError::Ok) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        uint8_t pin = doc["pin"];
        uint8_t value = doc["value"];
        
        if (digitalWrite(pin, value)) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid pin\"}");
        }
    });
    
    // Digital read
    server.on("/_api/gpio/read", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("pin")) {
            request->send(400, "application/json", "{\"error\":\"Missing pin parameter\"}");
            return;
        }
        
        uint8_t pin = request->getParam("pin")->value().toInt();
        int value = digitalRead(pin);
        
        if (value >= 0) {
            JsonDocument doc;
            doc["pin"] = pin;
            doc["value"] = value;
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid pin\"}");
        }
    });
    
    // Analog read
    server.on("/_api/gpio/analog", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("pin")) {
            request->send(400, "application/json", "{\"error\":\"Missing pin parameter\"}");
            return;
        }
        
        uint8_t pin = request->getParam("pin")->value().toInt();
        int value = analogRead(pin);
        
        if (value >= 0) {
            JsonDocument doc;
            doc["pin"] = pin;
            doc["value"] = value;
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json", "{\"error\":\"Pin does not support ADC\"}");
        }
    });
}

