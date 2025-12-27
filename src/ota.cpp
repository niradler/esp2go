#include "ota.h"
#include "config.h"
#include "hardware.h"
#include <Update.h>
#include <SD.h>
#include <WiFi.h>

static bool otaPending = false;
static String otaFirmwarePath = "";

void initOTA() {
  #ifdef OTA_PASSWORD
  LOG_INFO("🔄 OTA Updates: ENABLED (password protected)");
  #else
  LOG_INFO("🔄 OTA Updates: ENABLED (no password)");
  #endif
}

void scheduleOTAUpdate(const String& firmwarePath) {
  otaFirmwarePath = firmwarePath;
  otaPending = true;
  LOG_INFO("OTA update scheduled: %s", firmwarePath.c_str());
}

bool isOTAPending() {
  return otaPending;
}

void handleOTA() {
  if (!otaPending) return;
  
  otaPending = false;
  
  Serial.println("\n========================================");
  Serial.println("PERFORMING OTA UPDATE - STOPPING ALL SERVICES");
  Serial.println("========================================");
  
  LOG_INFO("Free heap before cleanup: %d bytes", ESP.getFreeHeap());
  
  stopRecording();
  WiFi.disconnect(false);
  delay(500);
  
  LOG_INFO("Free heap after cleanup: %d bytes", ESP.getFreeHeap());
  LOG_INFO("Starting OTA update from SD card: %s", otaFirmwarePath.c_str());
  
  File firmwareFile = SD.open(otaFirmwarePath, FILE_READ);
  if (!firmwareFile) {
    LOG_ERROR("Cannot open firmware file: %s", otaFirmwarePath.c_str());
    ESP.restart();
    return;
  }
  
  size_t fileSize = firmwareFile.size();
  LOG_INFO("Firmware file size: %d bytes (%d KB)", fileSize, fileSize / 1024);
  
  if (!Update.begin(fileSize)) {
    LOG_ERROR("OTA Update.begin() failed - not enough space");
    Update.printError(Serial);
    firmwareFile.close();
    ESP.restart();
    return;
  }
  
  LOG_INFO("OTA: Update partition ready, reading from SD card...");
  
  uint8_t *buffer = (uint8_t*)malloc(512);
  if (!buffer) {
    LOG_ERROR("Failed to allocate buffer!");
    firmwareFile.close();
    Update.abort();
    ESP.restart();
    return;
  }
  
  size_t totalRead = 0;
  
  while (firmwareFile.available()) {
    size_t bytesRead = firmwareFile.read(buffer, 512);
    
    if (bytesRead > 0) {
      if (Update.write(buffer, bytesRead) != bytesRead) {
        LOG_ERROR("OTA write failed at %d bytes", totalRead);
        Update.printError(Serial);
        free(buffer);
        firmwareFile.close();
        Update.abort();
        ESP.restart();
        return;
      }
      
      totalRead += bytesRead;
      
      if (totalRead % 102400 == 0) {
        LOG_INFO("OTA: Flashed %d KB / %d KB", totalRead / 1024, fileSize / 1024);
      }
    }
  }
  
  free(buffer);
  firmwareFile.close();
  LOG_INFO("OTA: All data written (%d bytes), finalizing...", totalRead);
  
  if (Update.end(true)) {
    Serial.println("\n==================================================");
    LOG_INFO("✅ OTA UPDATE SUCCESSFUL!");
    LOG_INFO("🔄 Restarting ESP32 with new firmware...");
    Serial.println("==================================================");
    
    delay(1000);
    ESP.restart();
  } else {
    LOG_ERROR("OTA Update.end() failed");
    Update.printError(Serial);
    ESP.restart();
  }
}

