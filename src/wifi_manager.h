#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

#define MAX_WIFI_NETWORKS 10
#define WIFI_CONFIG_FILE PATH_WIFI_CONFIG

struct WiFiNetwork {
  String ssid;
  String password;
  int priority;
};

extern WiFiNetwork savedNetworks[MAX_WIFI_NETWORKS];
extern int networkCount;
extern String saved_ap_ssid;
extern String saved_ap_password;

void initWiFiConfig();
void setupWiFi();

#endif
