#include "storage.h"
#include <SPI.h>

#define SDCARD_MISO 14
#define SDCARD_MOSI 17
#define SDCARD_SCK 42
#define SDCARD_CS 40

void setupSDCard() {
  Serial.println("ℹ️  INFO: Initializing SD card...");
  SPI.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  
  if (!SD.begin(SDCARD_CS)) {
    Serial.println("❌ ERROR: SD Card Mount Failed!");
    return;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("❌ ERROR: No SD card attached");
    return;
  }
  
  Serial.printf("ℹ️  INFO: SD Card Type: %s\n", 
    cardType == CARD_MMC ? "MMC" :
    cardType == CARD_SD ? "SDSC" :
    cardType == CARD_SDHC ? "SDHC" : "Unknown");
  
  Serial.printf("ℹ️  INFO: SD Card Size: %llu MB\n", SD.cardSize() / (1024 * 1024));
  Serial.printf("ℹ️  INFO: Used Space: %llu MB\n", SD.usedBytes() / (1024 * 1024));
}

void listDir(File dir, String path, JsonArray& files, int depth = 0, int& count = *(new int(0))) {
  if (depth > 3 || count > 500) {
    Serial.printf("⚠️  WARN: listDir limit reached: depth=%d, count=%d\n", depth, count);
    return;
  }
  
  File file = dir.openNextFile();
  while (file && count < 500) {
    JsonObject fileObj = files.add<JsonObject>();
    fileObj["name"] = String(file.name());
    fileObj["size"] = file.size();
    fileObj["isDir"] = file.isDirectory();
    fileObj["path"] = path + "/" + String(file.name());
    
    count++;
    
    if (file.isDirectory() && depth < 2) {
      listDir(file, path + "/" + String(file.name()), files, depth + 1, count);
    }
    file = dir.openNextFile();
    
    if (count % 10 == 0) {
      yield();
    }
  }
}

bool isSDCardMounted() {
  return SD.cardType() != CARD_NONE;
}

uint64_t getSDCardSize() {
  return SD.cardSize();
}

uint64_t getSDCardUsed() {
  return SD.usedBytes();
}

