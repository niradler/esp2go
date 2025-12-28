#include "config.h"
#include <SD.h>
#include <ArduinoJson.h>

ConfigManager::ConfigManager() : configLoaded(false) {
    setDefaults();
}

void ConfigManager::setDefaults() {
    usbHidEnabled = USB_HID_ENABLED;
    usbHidBootTimeout = USB_HID_BOOT_TIMEOUT;
    
    defaultWifiSsid = String(DEFAULT_WIFI_SSID);
    defaultWifiPassword = String(DEFAULT_WIFI_PASSWORD);
    defaultApSsid = String(DEFAULT_AP_SSID);
    defaultApPassword = String(DEFAULT_AP_PASSWORD);
    mdnsHostname = String(MDNS_HOSTNAME);
}

bool ConfigManager::loadConfig() {
    if (!SD.exists(PATH_CONFIG)) {
        LOG_INFO("Config file not found at %s, using defaults", PATH_CONFIG);
        return false;
    }
    
    File file = SD.open(PATH_CONFIG, FILE_READ);
    if (!file) {
        LOG_WARN("Failed to open config file: %s", PATH_CONFIG);
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Failed to parse config file: %s", error.c_str());
        return false;
    }
    
    LOG_INFO("Loading configuration from %s", PATH_CONFIG);
    
    if (doc["USB_HID_ENABLED"].is<bool>()) {
        usbHidEnabled = doc["USB_HID_ENABLED"].as<bool>();
        LOG_INFO("USB_HID_ENABLED: %s", usbHidEnabled ? "true" : "false");
    }
    
    if (doc["USB_HID_BOOT_TIMEOUT"].is<uint32_t>()) {
        usbHidBootTimeout = doc["USB_HID_BOOT_TIMEOUT"].as<uint32_t>();
        LOG_INFO("USB_HID_BOOT_TIMEOUT: %lu ms", usbHidBootTimeout);
    }
    
    if (doc["DEFAULT_WIFI_SSID"].is<const char*>()) {
        defaultWifiSsid = doc["DEFAULT_WIFI_SSID"].as<String>();
        LOG_INFO("DEFAULT_WIFI_SSID: %s", defaultWifiSsid.c_str());
    }
    
    if (doc["DEFAULT_WIFI_PASSWORD"].is<const char*>()) {
        defaultWifiPassword = doc["DEFAULT_WIFI_PASSWORD"].as<String>();
        LOG_INFO("DEFAULT_WIFI_PASSWORD: [hidden]");
    }
    
    if (doc["DEFAULT_AP_SSID"].is<const char*>()) {
        defaultApSsid = doc["DEFAULT_AP_SSID"].as<String>();
        LOG_INFO("DEFAULT_AP_SSID: %s", defaultApSsid.c_str());
    }
    
    if (doc["DEFAULT_AP_PASSWORD"].is<const char*>()) {
        defaultApPassword = doc["DEFAULT_AP_PASSWORD"].as<String>();
        LOG_INFO("DEFAULT_AP_PASSWORD: [hidden]");
    }
    
    if (doc["MDNS_HOSTNAME"].is<const char*>()) {
        mdnsHostname = doc["MDNS_HOSTNAME"].as<String>();
        LOG_INFO("MDNS_HOSTNAME: %s", mdnsHostname.c_str());
    }
    
    configLoaded = true;
    LOG_INFO("Configuration loaded successfully");
    return true;
}

