#ifndef MODULE_BASE_H
#define MODULE_BASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

/**
 * Base class for all ESP2GO modules
 * 
 * Each module represents a capability (USB HID, GPIO, LED, etc.)
 * and provides a consistent interface for initialization, API registration,
 * and lifecycle management.
 */
class ModuleBase {
public:
    virtual ~ModuleBase() {}
    
    /**
     * Get module name (e.g., "usb_hid", "gpio", "led")
     */
    virtual const char* getName() const = 0;
    
    /**
     * Get module version
     */
    virtual const char* getVersion() const { return "1.0.0"; }
    
    /**
     * Get module description
     */
    virtual const char* getDescription() const = 0;
    
    /**
     * Initialize the module
     * Called during system startup
     * @return true if initialization succeeded
     */
    virtual bool init() = 0;
    
    /**
     * Register API endpoints with the web server
     * All endpoints should follow the pattern: /_api/{module_name}/{action}
     * @param server The web server instance
     */
    virtual void registerAPI(AsyncWebServer& server) = 0;
    
    /**
     * Update/loop function called regularly
     * Use for polling, state updates, etc.
     */
    virtual void update() {}
    
    /**
     * Check if module is enabled
     */
    virtual bool isEnabled() const { return true; }
    
    /**
     * Check if module is ready/available
     */
    virtual bool isReady() const = 0;
    
    /**
     * Get module status as JSON
     * Should include at least: enabled, ready, name, version
     */
    virtual void getStatus(JsonObject& obj) const {
        obj["name"] = getName();
        obj["version"] = getVersion();
        obj["enabled"] = isEnabled();
        obj["ready"] = isReady();
        obj["description"] = getDescription();
    }
    
    /**
     * Shutdown/cleanup the module
     */
    virtual void shutdown() {}
    
protected:
    /**
     * Log helper functions
     */
    void logInfo(const char* format, ...) const {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.printf("ℹ️  [%s] %s\n", getName(), buffer);
    }
    
    void logWarn(const char* format, ...) const {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.printf("⚠️  [%s] %s\n", getName(), buffer);
    }
    
    void logError(const char* format, ...) const {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.printf("❌ [%s] %s\n", getName(), buffer);
    }
};

#endif

