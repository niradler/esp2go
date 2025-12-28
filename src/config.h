#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASSWORD
#define DEFAULT_WIFI_PASSWORD ""
#endif
#ifndef DEFAULT_AP_SSID
#define DEFAULT_AP_SSID "ESP2GO"
#endif
#ifndef DEFAULT_AP_PASSWORD
#define DEFAULT_AP_PASSWORD "12345678"
#endif

#ifndef MDNS_HOSTNAME
#define MDNS_HOSTNAME "esp2go"
#endif

#ifndef USB_HID_ENABLED
#define USB_HID_ENABLED false
#endif
#ifndef USB_HID_BOOT_TIMEOUT
#define USB_HID_BOOT_TIMEOUT 3000
#endif

#define PATH_INDEX "/index.html"
#define PATH_WIFI_CONFIG "/os/wifi_config.json"
#define PATH_CONFIG "/os/config.json"
#define PATH_OTA_UPDATE "/os/ota_update.html"
#define PATH_FIRMWARE_DEFAULT "/os/firmware.bin"

#define DIR_APPS "/apps"
#define DIR_DOCS "/docs"
#define DIR_OS "/os"

#define LOG_TAG "[ESP2GO]"
#define LOG_ERROR(fmt, ...) Serial.printf("❌ ERROR %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) Serial.printf("⚠️  WARN %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) Serial.printf("ℹ️  INFO %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) Serial.printf("🔍 DEBUG %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool loadConfig();
    
    bool getUsbHidEnabled() const { return usbHidEnabled; }
    uint32_t getUsbHidBootTimeout() const { return usbHidBootTimeout; }
    
    const String& getDefaultWifiSsid() const { return defaultWifiSsid; }
    const String& getDefaultWifiPassword() const { return defaultWifiPassword; }
    const String& getDefaultApSsid() const { return defaultApSsid; }
    const String& getDefaultApPassword() const { return defaultApPassword; }
    const String& getMdnsHostname() const { return mdnsHostname; }
    
    bool isConfigLoaded() const { return configLoaded; }

private:
    ConfigManager();
    void setDefaults();
    
    bool configLoaded;
    
    bool usbHidEnabled;
    uint32_t usbHidBootTimeout;
    
    String defaultWifiSsid;
    String defaultWifiPassword;
    String defaultApSsid;
    String defaultApPassword;
    String mdnsHostname;
};

#define Config ConfigManager::getInstance()

#endif

