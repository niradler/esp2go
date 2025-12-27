#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>
#include "config.h"
#include "storage.h"
#include "hardware.h"
#include "wifi_config.h"
#include "server.h"
#include "ota.h"

void printSystemInfo() {
  Serial.println("\n==================================================");
  LOG_INFO("ESP32 Web Storage System Starting...");
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
  
  setupSDCard();
  setupMicrophone();
  initWiFiConfig();
  setupWiFi();
  setupWebServer();
  initOTA();
  
  Serial.println("==================================================");
  LOG_INFO("System Ready!");
  LOG_INFO("Free Heap After Init: %d bytes", ESP.getFreeHeap());
  Serial.println("==================================================\n");
}

void monitorWiFi() {
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  
  if (now - lastCheck > 30000) {
    lastCheck = now;
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
      LOG_WARN("WiFi disconnected, attempting reconnect...");
      WiFi.reconnect();
    }
  }
}

void monitorHeap() {
  static unsigned long lastHeapCheck = 0;
  unsigned long now = millis();
  
  if (now - lastHeapCheck > 90000) {
    lastHeapCheck = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < 10000) {
      LOG_ERROR("Low memory warning! Free heap: %d bytes", freeHeap);
    }
  }
}

void loop() {
  handleOTA();
  
  if (!isOTAPending()) {
    processRecording();
    monitorWiFi();
    monitorHeap();
  }
  
  delay(100);
  yield();
}
