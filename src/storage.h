#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>

// SD Card initialization
void setupSDCard();

// SD Card operations
void listDir(File dir, String path, JsonArray& files, int depth, int& count);
bool isSDCardMounted();
uint64_t getSDCardSize();
uint64_t getSDCardUsed();

#endif

