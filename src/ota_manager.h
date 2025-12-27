#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

void initOTA();
void handleOTA();
void scheduleOTAUpdate(const String& firmwarePath);
bool isOTAPending();

#endif

