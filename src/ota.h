#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

void initOTA();
void handleOTA();
void scheduleOTAUpdate(const String& firmwarePath);
bool isOTAPending();

#endif

