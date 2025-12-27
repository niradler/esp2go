#include "wifi_manager.h"
#include "config.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

WiFiNetwork savedNetworks[MAX_WIFI_NETWORKS];
int networkCount = 0;
String saved_ap_ssid = DEFAULT_AP_SSID;
String saved_ap_password = DEFAULT_AP_PASSWORD;

bool loadWiFiConfig() {
  if (!SD.exists(WIFI_CONFIG_FILE)) {
    LOG_WARN("WiFi config file not found on SD card");
    return false;
  }
  
  File file = SD.open(WIFI_CONFIG_FILE, FILE_READ);
  if (!file) {
    LOG_ERROR("Failed to open WiFi config file");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    LOG_ERROR("Failed to parse WiFi config: %s", error.c_str());
    return false;
  }
  
  networkCount = 0;
  JsonArray networks = doc["networks"];
  for (JsonObject net : networks) {
    if (networkCount >= MAX_WIFI_NETWORKS) break;
    
    savedNetworks[networkCount].ssid = net["ssid"].as<String>();
    savedNetworks[networkCount].password = net["password"].as<String>();
    savedNetworks[networkCount].priority = net["priority"] | (networkCount + 1);
    
    if (savedNetworks[networkCount].ssid.length() > 0) {
      LOG_INFO("Loaded network %d: %s (priority: %d)", 
        networkCount + 1, 
        savedNetworks[networkCount].ssid.c_str(),
        savedNetworks[networkCount].priority);
      networkCount++;
    }
  }
  
  if (doc["ap_ssid"].is<String>()) {
    saved_ap_ssid = doc["ap_ssid"].as<String>();
  }
  if (doc["ap_password"].is<String>()) {
    saved_ap_password = doc["ap_password"].as<String>();
  }
  
  LOG_INFO("Loaded %d WiFi networks from config", networkCount);
  return networkCount > 0;
}

bool saveWiFiConfig() {
  JsonDocument doc;
  JsonArray networks = doc["networks"].to<JsonArray>();
  
  for (int i = 0; i < networkCount; i++) {
    JsonObject net = networks.add<JsonObject>();
    net["ssid"] = savedNetworks[i].ssid;
    net["password"] = savedNetworks[i].password;
    net["priority"] = savedNetworks[i].priority;
  }
  
  doc["ap_ssid"] = saved_ap_ssid;
  doc["ap_password"] = saved_ap_password;
  
  File file = SD.open(WIFI_CONFIG_FILE, FILE_WRITE);
  if (!file) {
    LOG_ERROR("Failed to open WiFi config for writing");
    return false;
  }
  
  serializeJsonPretty(doc, file);
  file.close();
  
  LOG_INFO("Saved %d WiFi networks to config", networkCount);
  return true;
}

void scanAndSelectWiFi() {
  Serial.println("\n==================================================");
  Serial.println("📡 WiFi Network Scanner & Setup");
  Serial.println("==================================================");
  
  LOG_INFO("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    LOG_WARN("No networks found");
    return;
  }
  
  LOG_INFO("Found %d networks:", n);
  Serial.println("\n ID | SSID                          | Signal | Channel | Security");
  Serial.println("----+-------------------------------+--------+---------+----------");
  
  for (int i = 0; i < n; i++) {
    Serial.printf(" %2d | %-29s | %3d dBm |   %2d    | %s\n",
      i + 1,
      WiFi.SSID(i).c_str(),
      WiFi.RSSI(i),
      WiFi.channel(i),
      WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" :
      WiFi.encryptionType(i) == WIFI_AUTH_WEP ? "WEP" :
      WiFi.encryptionType(i) == WIFI_AUTH_WPA_PSK ? "WPA" :
      WiFi.encryptionType(i) == WIFI_AUTH_WPA2_PSK ? "WPA2" :
      WiFi.encryptionType(i) == WIFI_AUTH_WPA_WPA2_PSK ? "WPA/WPA2" : "Other");
  }
  
  Serial.println("\nEnter network number (1-" + String(n) + ") or 0 to skip: ");
  
  while (!Serial.available()) {
    delay(100);
  }
  
  int selection = Serial.parseInt();
  while (Serial.available()) Serial.read();
  
  if (selection < 1 || selection > n) {
    LOG_INFO("Skipped network selection");
    return;
  }
  
  String selectedSSID = WiFi.SSID(selection - 1);
  LOG_INFO("Selected: %s", selectedSSID.c_str());
  
  Serial.print("Enter password (or press Enter for open network): ");
  String password = "";
  unsigned long timeout = millis() + 60000;
  
  while (millis() < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
      password += c;
      Serial.print('*');
    }
    delay(10);
  }
  Serial.println();
  
  if (networkCount < MAX_WIFI_NETWORKS) {
    savedNetworks[networkCount].ssid = selectedSSID;
    savedNetworks[networkCount].password = password;
    savedNetworks[networkCount].priority = networkCount + 1;
    networkCount++;
    
    if (saveWiFiConfig()) {
      LOG_INFO("Network saved successfully!");
    }
  } else {
    LOG_WARN("Maximum networks reached (%d)", MAX_WIFI_NETWORKS);
  }
}

void initWiFiConfig() {
  if (SD.cardType() == CARD_NONE) {
    LOG_WARN("Cannot init WiFi config - SD card not available");
    return;
  }
  
  if (SD.exists(WIFI_CONFIG_FILE)) {
    LOG_INFO("WiFi config file exists");
    return;
  }
  
  LOG_INFO("Creating default WiFi config file...");
  
  // Ensure /os directory exists
  if (!SD.exists(DIR_OS)) {
    if (!SD.mkdir(DIR_OS)) {
      LOG_ERROR("Failed to create %s directory", DIR_OS);
      return;
    }
    LOG_INFO("Created %s directory", DIR_OS);
  }
  
  JsonDocument doc;
  JsonArray networks = doc["networks"].to<JsonArray>();
  
  JsonObject net = networks.add<JsonObject>();
  net["ssid"] = "";
  net["password"] = "";
  net["priority"] = 1;
  
  doc["ap_ssid"] = DEFAULT_AP_SSID;
  doc["ap_password"] = DEFAULT_AP_PASSWORD;
  
  File file = SD.open(WIFI_CONFIG_FILE, FILE_WRITE);
  if (!file) {
    LOG_ERROR("Failed to create WiFi config file");
    return;
  }
  
  serializeJsonPretty(doc, file);
  file.close();
  
  LOG_INFO("Created WiFi config file: %s", WIFI_CONFIG_FILE);
}

void setupWiFi() {
  LOG_INFO("Starting WiFi setup...");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  bool hasConfig = loadWiFiConfig();
  bool connected = false;
  
  if (!hasConfig || networkCount == 0) {
    LOG_WARN("No saved WiFi networks. Starting interactive setup...");
    Serial.println("\nPress 'S' within 10 seconds to scan and setup WiFi...");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'S' || c == 's') {
          scanAndSelectWiFi();
          hasConfig = loadWiFiConfig();
          break;
        }
      }
      delay(100);
    }
  }
  
  if (hasConfig && networkCount > 0) {
    for (int priority = 1; priority <= networkCount && !connected; priority++) {
      for (int i = 0; i < networkCount && !connected; i++) {
        if (savedNetworks[i].priority != priority) continue;
        
        LOG_INFO("Attempting to connect to: %s (priority %d)", 
          savedNetworks[i].ssid.c_str(), savedNetworks[i].priority);
        
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        WiFi.begin(savedNetworks[i].ssid.c_str(), savedNetworks[i].password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          Serial.print(".");
          attempts++;
          yield();
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
          connected = true;
          LOG_INFO("WiFi Connected Successfully!");
          LOG_INFO("SSID: %s", savedNetworks[i].ssid.c_str());
          LOG_INFO("IP Address: %s", WiFi.localIP().toString().c_str());
          LOG_INFO("Gateway: %s", WiFi.gatewayIP().toString().c_str());
          LOG_INFO("DNS: %s", WiFi.dnsIP().toString().c_str());
          LOG_INFO("Signal Strength: %d dBm", WiFi.RSSI());
          LOG_INFO("Channel: %d", WiFi.channel());
          LOG_INFO("MAC Address: %s", WiFi.macAddress().c_str());
          
          if (MDNS.begin(MDNS_HOSTNAME)) {
            LOG_INFO("mDNS responder started: http://%s.local", MDNS_HOSTNAME);
          } else {
            LOG_WARN("Failed to start mDNS responder");
          }
        } else {
          LOG_WARN("Failed to connect to: %s", savedNetworks[i].ssid.c_str());
        }
      }
    }
  }
  
  if (!connected) {
    LOG_INFO("Fallback: Trying default config from config.h...");
    WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
      yield();
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      LOG_INFO("Connected to default network: %s", DEFAULT_WIFI_SSID);
      LOG_INFO("IP Address: %s", WiFi.localIP().toString().c_str());
      LOG_INFO("Gateway: %s", WiFi.gatewayIP().toString().c_str());
      LOG_INFO("DNS: %s", WiFi.dnsIP().toString().c_str());
      LOG_INFO("Signal Strength: %d dBm", WiFi.RSSI());
      LOG_INFO("Channel: %d", WiFi.channel());
      LOG_INFO("MAC Address: %s", WiFi.macAddress().c_str());
      
      if (MDNS.begin(MDNS_HOSTNAME)) {
        LOG_INFO("mDNS responder started: http://%s.local", MDNS_HOSTNAME);
      } else {
        LOG_WARN("Failed to start mDNS responder");
      }
    }
  }
  
  if (!connected) {
    LOG_WARN("All WiFi connection attempts failed");
    LOG_INFO("Starting Access Point mode...");
    
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    
    if (WiFi.softAP(saved_ap_ssid.c_str(), saved_ap_password.c_str())) {
      IPAddress IP = WiFi.softAPIP();
      LOG_INFO("AP Mode Started Successfully");
      LOG_INFO("AP IP Address: %s", IP.toString().c_str());
      LOG_INFO("AP SSID: %s", saved_ap_ssid.c_str());
      LOG_INFO("AP Password: %s", saved_ap_password.c_str());
      LOG_INFO("AP MAC Address: %s", WiFi.softAPmacAddress().c_str());
    } else {
      LOG_ERROR("Failed to start Access Point mode!");
    }
  }
}
