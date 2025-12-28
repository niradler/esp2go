#include "usb_hid_module.h"
#include "../config.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "USBHIDConsumerControl.h"

static USBHIDKeyboard* keyboard = nullptr;
static USBHIDMouse* mouse = nullptr;
static USBHIDConsumerControl* consumer = nullptr;

bool USBHIDModule::init() {
    logInfo("Initializing...");
    
    if (!Config.getUsbHidEnabled()) {
        logWarn("USB HID disabled in config");
        return false;
    }
    
    initialized = true;
    isActive = false;
    logInfo("Initialized (disabled by default - use API to enable)");
    logWarn("Note: Enabling USB HID will disable Serial/JTAG debugging!");
    return true;
}

void USBHIDModule::update() {
}

bool USBHIDModule::enableHID() {
    if (!initialized) {
        logError("Module not initialized");
        return false;
    }
    
    if (isActive) {
        logWarn("USB HID already active");
        return true;
    }
    
    try {
        logWarn("⚠️  ENABLING USB HID - Serial port will become unavailable!");
        logWarn("⚠️  To recover: Hold button during boot for safe mode");
        delay(1000); // Give user time to see warning
        
        USB.begin();
        
        keyboard = new USBHIDKeyboard();
        mouse = new USBHIDMouse();
        consumer = new USBHIDConsumerControl();
        
        keyboard->begin();
        mouse->begin();
        consumer->begin();
        
        isActive = true;
        logInfo("✅ USB HID enabled successfully");
        return true;
    } catch (...) {
        logError("Failed to enable USB HID");
        if (keyboard) delete keyboard;
        if (mouse) delete mouse;
        if (consumer) delete consumer;
        keyboard = nullptr;
        mouse = nullptr;
        consumer = nullptr;
        isActive = false;
        return false;
    }
}

bool USBHIDModule::disableHID() {
    if (!isActive) {
        logInfo("USB HID already disabled");
        return true;
    }
    
    logInfo("Disabling USB HID...");
    
    if (keyboard) {
        keyboard->end();
        delete keyboard;
        keyboard = nullptr;
    }
    if (mouse) {
        mouse->end();
        delete mouse;
        mouse = nullptr;
    }
    if (consumer) {
        consumer->end();
        delete consumer;
        consumer = nullptr;
    }
    
    // Note: USB.end() doesn't exist in ESP32 Arduino
    // The USB devices are released but USB peripheral stays active
    isActive = false;
    
    logWarn("⚠️  USB HID disabled - restart device recommended");
    return true;
}

void USBHIDModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["active"] = isActive;
    obj["keyboard_ready"] = keyboard != nullptr;
    obj["mouse_ready"] = mouse != nullptr;
    obj["consumer_ready"] = consumer != nullptr;
    obj["serial_port_available"] = !isActive;
    obj["warning"] = "Enabling USB HID disables Serial/JTAG debugging";
    obj["config_enabled"] = Config.getUsbHidEnabled();
}

void USBHIDModule::registerAPI(AsyncWebServer& server) {
    // Status endpoint
    server.on("/_api/hid/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getStatus(obj);
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Enable USB HID
    server.on("/_api/hid/enable", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (enableHID()) {
            request->send(200, "application/json", 
                "{\"status\":\"enabled\","
                "\"warning\":\"Serial port is now unavailable. Hold button during boot to disable.\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to enable USB HID\"}");
        }
    });
    
    // Disable USB HID
    server.on("/_api/hid/disable", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (disableHID()) {
            request->send(200, "application/json", 
                "{\"status\":\"disabled\","
                "\"note\":\"Restart device to restore Serial port\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to disable USB HID\"}");
        }
    });
    
    // Type text
    server.on("/_api/hid/keyboard/text", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !keyboard) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            if (deserializeJson(doc, data, len) || !doc["text"].is<const char*>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid text parameter\"}");
                return;
            }
            
            const char* text = doc["text"].as<const char*>();
            keyboard->print(text);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    // Send key with modifiers
    server.on("/_api/hid/keyboard/key", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !keyboard) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            if (deserializeJson(doc, data, len) || !doc["key"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid key parameter\"}");
                return;
            }
            
            uint8_t key = doc["key"].as<int>();
            uint8_t modifiers = doc["modifiers"].is<int>() ? doc["modifiers"].as<int>() : 0;
            
            // Press modifiers
            if (modifiers & 0x01) keyboard->press(KEY_LEFT_CTRL);
            if (modifiers & 0x02) keyboard->press(KEY_LEFT_SHIFT);
            if (modifiers & 0x04) keyboard->press(KEY_LEFT_ALT);
            if (modifiers & 0x08) keyboard->press(KEY_LEFT_GUI);
            
            // Press and release key
            keyboard->press(key);
            delay(10);
            keyboard->releaseAll();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    // Mouse move
    server.on("/_api/hid/mouse/move", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !mouse) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            if (deserializeJson(doc, data, len) || !doc["x"].is<int>() || !doc["y"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid x, y parameters\"}");
                return;
            }
            
            int8_t x = doc["x"].as<int>();
            int8_t y = doc["y"].as<int>();
            
            mouse->move(x, y);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    // Mouse click
    server.on("/_api/hid/mouse/click", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !mouse) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            deserializeJson(doc, data, len);
            uint8_t button = doc["button"].is<int>() ? doc["button"].as<int>() : MOUSE_LEFT;
            
            mouse->click(button);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    // Mouse scroll
    server.on("/_api/hid/mouse/scroll", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !mouse) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            if (deserializeJson(doc, data, len) || !doc["amount"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid amount parameter\"}");
                return;
            }
            
            int8_t amount = doc["amount"].as<int>();
            mouse->move(0, 0, amount);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    // Media control
    server.on("/_api/hid/media", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isActive || !consumer) {
                request->send(503, "application/json", 
                    "{\"error\":\"USB HID not active\","
                    "\"hint\":\"Call POST /_api/hid/enable first\"}");
                return;
            }
            
            JsonDocument doc;
            if (deserializeJson(doc, data, len) || !doc["key"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid key parameter\"}");
                return;
            }
            
            uint16_t key = doc["key"].as<int>();
            consumer->press(key);
            delay(50);
            consumer->release();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
    
    logInfo("API endpoints registered (HID disabled by default)");
}
