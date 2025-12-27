#ifndef CONFIG_H
#define CONFIG_H

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

#define PATH_INDEX "/index.html"
#define PATH_WIFI_CONFIG "/os/wifi_config.json"
#define PATH_OTA_UPDATE "/os/ota_update.html"
#define PATH_FIRMWARE_DEFAULT "/firmware.bin"

#define DIR_APPS "/apps"
#define DIR_DOCS "/docs"
#define DIR_OS "/os"

#define LOG_TAG "[ESP2GO]"
#define LOG_ERROR(fmt, ...) Serial.printf("❌ ERROR %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) Serial.printf("⚠️  WARN %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) Serial.printf("ℹ️  INFO %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) Serial.printf("🔍 DEBUG %s: " fmt "\n", LOG_TAG, ##__VA_ARGS__)

#endif

