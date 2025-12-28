#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"
#include "hardware.h"
#include "wifi_manager.h"
#include "api_server.h"
#include "modules/module_manager.h"
#include "modules/system_module.h"
#include "modules/storage_module.h"
#include "modules/led_module.h"
#include "modules/microphone_module.h"
#include "modules/button_module.h"
#include "modules/gpio_module.h"
#include "modules/usb_hid_module.h"
#include "modules/ota_module.h"
#include "modules/web_server_module.h"

#define SDCARD_MISO 14
#define SDCARD_MOSI 17
#define SDCARD_SCK 42
#define SDCARD_CS 40

void printSystemInfo() {
  Serial.println("\n==================================================");
  LOG_INFO("ESP2GO System Starting...");
  LOG_INFO("🔄 Firmware Version: %s", __DATE__ " " __TIME__);
  Serial.println("==================================================");
  
  esp_reset_reason_t reset_reason = esp_reset_reason();
  LOG_INFO("Reset Reason: %s", 
    reset_reason == ESP_RST_POWERON ? "Power On" :
    reset_reason == ESP_RST_SW ? "Software Reset" :
    reset_reason == ESP_RST_PANIC ? "PANIC/Exception" :
    reset_reason == ESP_RST_INT_WDT ? "Interrupt Watchdog" :
    reset_reason == ESP_RST_TASK_WDT ? "Task Watchdog" :
    reset_reason == ESP_RST_WDT ? "Watchdog" :
    reset_reason == ESP_RST_DEEPSLEEP ? "Deep Sleep" :
    reset_reason == ESP_RST_BROWNOUT ? "Brownout" :
    reset_reason == ESP_RST_SDIO ? "SDIO" : "Unknown");
  
  LOG_INFO("ESP-IDF Version: %s", esp_get_idf_version());
  LOG_INFO("Chip Model: %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
  LOG_INFO("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
  LOG_INFO("Flash Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
  LOG_INFO("Free Heap: %d bytes", ESP.getFreeHeap());
  LOG_INFO("Sketch Size: %d bytes", ESP.getSketchSize());
  LOG_INFO("Free Sketch Space: %d bytes", ESP.getFreeSketchSpace());
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  Serial.begin(115200);
  delay(1000);
  
  printSystemInfo();
  
  // Initialize SPI for SD card (needed for config loading)
  LOG_INFO("Initializing SPI for SD card...");
  SPI.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  
  // Initialize SD card and load configuration
  if (SD.begin()) {
    LOG_INFO("SD card initialized, loading configuration...");
    Config.loadConfig();
  } else {
    LOG_WARN("SD card not available, using default configuration");
  }
  
  // USB HID safe mode check (must happen before USB init)
  bool skipUSB = false;
  
  if (Config.getUsbHidEnabled()) {
    LOG_INFO("Checking for USB HID safe mode...");
    LOG_INFO("Hold BUTTON for %lu seconds to skip USB HID initialization", Config.getUsbHidBootTimeout() / 1000);
    
    unsigned long startTime = millis();
    int buttonPressCount = 0;
    
    while (millis() - startTime < Config.getUsbHidBootTimeout()) {
      M5.update();
      if (M5.BtnA.isPressed()) {
        buttonPressCount++;
        if (buttonPressCount > 5) {
          skipUSB = true;
          LOG_WARN("⚠️  SAFE MODE ACTIVATED!");
          LOG_WARN("⚠️  USB HID will be DISABLED for this session");
          LOG_INFO("✅ You can now flash firmware safely");
          
          for (int i = 0; i < 6; i++) {
            setLED(i % 2 ? 255 : 0, i % 2 ? 0 : 255, 0);
            delay(100);
          }
          setLED(0, 0, 0);
          break;
        }
      } else {
        buttonPressCount = 0;
      }
      delay(100);
    }
    
    if (!skipUSB) {
      LOG_INFO("No safe mode requested");
    }
  }
  
  setupMicrophone();
  
  // Register all modules
  LOG_INFO("Registering modules...");
  ModuleManager& mgr = ModuleManager::getInstance();
  
  mgr.registerModule(new SystemModule());
  mgr.registerModule(new StorageModule());
  mgr.registerModule(new LEDModule());
  mgr.registerModule(new MicrophoneModule());
  mgr.registerModule(new ButtonModule());
  mgr.registerModule(new GPIOModule());
  
  if (Config.getUsbHidEnabled() && !skipUSB) {
    mgr.registerModule(new USBHIDModule());
  }
  
  mgr.registerModule(new OTAModule());
  mgr.registerModule(new WebServerModule());
  
  // Initialize all modules
  LOG_INFO("Initializing modules...");
  mgr.initAll();
  
  // Setup WiFi
  initWiFiConfig();
  setupWiFi();
  
  // Setup web server with all module APIs
  setupWebServer();
  
  LOG_INFO("==================================================");
  LOG_INFO("✅ ESP2GO System Ready!");
  LOG_INFO("📡 Access via: http://%s.local", Config.getMdnsHostname().c_str());
  LOG_INFO("📡 Or via IP: http://%s", WiFi.localIP().toString().c_str());
  LOG_INFO("==================================================");
  
  setLED(0, 255, 0);
  delay(500);
  setLED(0, 0, 0);
}

void loop() {
  M5.update();
  
  // Update all modules
  ModuleManager::getInstance().updateAll();
  
  delay(10);
}
