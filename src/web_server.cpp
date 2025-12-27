#include "web_server.h"
#include "config.h"
#include "system_control.h"
#include "storage_manager.h"
#include "ota_manager.h"
#include <WiFi.h>
#include <SD.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);

AsyncWebServer& getWebServer() {
  return server;
}

void stopWebServer() {
  server.end();
}

void setupAPIEndpoints() {
  server.on("/_api/system/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["chip"] = ESP.getChipModel();
    doc["revision"] = ESP.getChipRevision();
    doc["cpu_freq"] = ESP.getCpuFreqMHz();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["connected"] = WiFi.status() == WL_CONNECTED;
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["mac"] = WiFi.macAddress();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/led/set", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      deserializeJson(doc, data, len);
      
      int r = doc["r"] | 0;
      int g = doc["g"] | 0;
      int b = doc["b"] | 0;
      
      rgbLedWrite(35, r, g, b);
      
      LOG_INFO("LED set to RGB(%d, %d, %d)", r, g, b);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  server.on("/_api/mic/level", HTTP_GET, [](AsyncWebServerRequest *request) {
    int level = readMicrophoneLevel();
    
    JsonDocument doc;
    doc["level"] = level;
    doc["initialized"] = isMicrophoneInitialized();
    doc["recording"] = isRecording();
    if (isRecording()) {
      doc["duration"] = getRecordingDuration();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/mic/record/start", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      deserializeJson(doc, data, len);
      
      String filename = doc["filename"] | "recording.wav";
      if (!filename.endsWith(".wav")) {
        filename += ".wav";
      }
      
      bool success = startRecording(filename.c_str());
      
      if (success) {
        LOG_INFO("Recording started: %s", filename.c_str());
        request->send(200, "application/json", "{\"status\":\"recording\"}");
      } else {
        request->send(500, "application/json", "{\"error\":\"Failed to start recording\"}");
      }
    });
  
  server.on("/_api/mic/record/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    stopRecording();
    LOG_INFO("Recording stopped");
    request->send(200, "application/json", "{\"status\":\"stopped\"}");
  });
  
  server.on("/_api/mic/record/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["recording"] = isRecording();
    if (isRecording()) {
      doc["duration"] = getRecordingDuration();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/button/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    pinMode(41, INPUT_PULLUP);
    bool pressed = digitalRead(41) == LOW;
    
    JsonDocument doc;
    doc["pressed"] = pressed;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
}

void setupGPIOEndpoints() {
  server.on("/_api/gpio/mode", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      deserializeJson(doc, data, len);
      
      if (!doc["pin"].is<int>() || !doc["mode"].is<const char*>()) {
        request->send(400, "application/json", "{\"error\":\"Missing pin or mode\"}");
        return;
      }
      
      int pin = doc["pin"];
      String mode = doc["mode"].as<String>();
      
      if (isReservedPin(pin)) {
        request->send(403, "application/json", "{\"error\":\"Pin is reserved for system use\"}");
        return;
      }
      
      if (mode == "INPUT") {
        pinMode(pin, INPUT);
      } else if (mode == "INPUT_PULLUP") {
        pinMode(pin, INPUT_PULLUP);
      } else if (mode == "INPUT_PULLDOWN") {
        pinMode(pin, INPUT_PULLDOWN);
      } else if (mode == "OUTPUT") {
        pinMode(pin, OUTPUT);
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid mode\"}");
        return;
      }
      
      LOG_INFO("GPIO %d set to mode: %s", pin, mode.c_str());
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  server.on("/_api/gpio/write", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      deserializeJson(doc, data, len);
      
      if (!doc["pin"].is<int>() || !doc["value"].is<int>()) {
        request->send(400, "application/json", "{\"error\":\"Missing pin or value\"}");
        return;
      }
      
      int pin = doc["pin"];
      int value = doc["value"];
      
      if (isReservedPin(pin)) {
        request->send(403, "application/json", "{\"error\":\"Pin is reserved for system use\"}");
        return;
      }
      
      digitalWrite(pin, value ? HIGH : LOW);
      LOG_INFO("GPIO %d set to: %d", pin, value);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  server.on("/_api/gpio/read", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("pin")) {
      request->send(400, "application/json", "{\"error\":\"Missing pin parameter\"}");
      return;
    }
    
    int pin = request->getParam("pin")->value().toInt();
    int value = digitalRead(pin);
    
    JsonDocument doc;
    doc["pin"] = pin;
    doc["value"] = value;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/gpio/analog", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("pin")) {
      request->send(400, "application/json", "{\"error\":\"Missing pin parameter\"}");
      return;
    }
    
    int pin = request->getParam("pin")->value().toInt();
    int value = analogRead(pin);
    
    JsonDocument doc;
    doc["pin"] = pin;
    doc["value"] = value;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/gpio/pins", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray available = doc["available"].to<JsonArray>();
    JsonArray reserved = doc["reserved"].to<JsonArray>();
    
    reserved.add(35);
    reserved.add(38);
    reserved.add(39);
    reserved.add(41);
    reserved.add(SDCARD_MISO);
    reserved.add(SDCARD_MOSI);
    reserved.add(SDCARD_SCK);
    reserved.add(SDCARD_CS);
    
    int availablePins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 18, 21, 43, 44, 45, 46, 47, 48};
    for (int pin : availablePins) {
      available.add(pin);
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
}

void setupFileEndpoints() {
  server.on("/_api/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    String path = request->hasParam("path") ? request->getParam("path")->value() : "/";
    
    if (!SD.exists(path)) {
      request->send(404, "application/json", "{\"error\":\"Path not found\"}");
      return;
    }
    
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
      dir.close();
      request->send(400, "application/json", "{\"error\":\"Not a directory\"}");
      return;
    }
    
    JsonDocument doc;
    JsonArray files = doc["files"].to<JsonArray>();
    
    File file = dir.openNextFile();
    while (file) {
      JsonObject fileObj = files.add<JsonObject>();
      fileObj["name"] = String(file.name());
      fileObj["size"] = file.size();
      fileObj["isDir"] = file.isDirectory();
      file = dir.openNextFile();
      yield();
    }
    dir.close();
    
    doc["path"] = path;
    doc["count"] = files.size();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/files/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      request->send(400, "application/json", "{\"error\":\"Missing path\"}");
      return;
    }
    
    String path = request->getParam("path")->value();
    if (!SD.exists(path)) {
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }
    
    File file = SD.open(path);
    JsonDocument doc;
    doc["name"] = String(file.name());
    doc["size"] = file.size();
    doc["isDir"] = file.isDirectory();
    doc["path"] = path;
    file.close();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/_api/files/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      request->send(400, "application/json", "{\"error\":\"Missing path\"}");
      return;
    }
    
    String path = request->getParam("path")->value();
    if (SD.remove(path)) {
      request->send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
      request->send(500, "application/json", "{\"error\":\"Delete failed\"}");
    }
  });
  
  server.on("/_api/files/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      LOG_INFO("/_api/files/upload: POST handler called - upload complete");
      request->send(200, "application/json", "{\"status\":\"uploaded\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      static File uploadFile;
      static String uploadPath;
      static size_t totalUploaded = 0;
      static bool uploadInProgress = false;
      
      if (index == 0) {
        if (uploadInProgress) {
          LOG_ERROR("/_api/files/upload: Upload already in progress, rejecting");
          return;
        }
        
        uploadInProgress = true;
        
        Serial.println("\n========================================");
        Serial.println("API UPLOAD STARTED");
        Serial.println("========================================");
        
        String path = request->hasParam("path") ? request->getParam("path")->value() : "/";
        uploadPath = path + "/" + filename;
        if (uploadPath.startsWith("//")) {
          uploadPath = uploadPath.substring(1);
        }
        totalUploaded = 0;
        
        if (uploadFile) {
          uploadFile.close();
        }
        
        if (SD.exists(uploadPath)) {
          SD.remove(uploadPath);
        }
        
        uploadFile = SD.open(uploadPath, FILE_WRITE);
        if (!uploadFile) {
          LOG_ERROR("/_api/files/upload: Cannot open file: %s", uploadPath.c_str());
          uploadInProgress = false;
          return;
        }
        LOG_INFO("/_api/files/upload: Started: %s", uploadPath.c_str());
      }
      
      if (len) {
        if (!uploadInProgress) {
          LOG_ERROR("/_api/files/upload: Chunk received but no upload in progress!");
          return;
        }
        
        if (!uploadFile || !uploadFile.available() == -1) {
          LOG_ERROR("/_api/files/upload: File handle invalid at chunk %d!", index);
          uploadInProgress = false;
          return;
        }
        
        size_t written = uploadFile.write(data, len);
        totalUploaded += written;
        
        if (written != len) {
          LOG_ERROR("/_api/files/upload: Write error at chunk %d - wrote %d of %d bytes", index, written, len);
          uploadFile.close();
          SD.remove(uploadPath);
          uploadInProgress = false;
          return;
        }
        
        if (totalUploaded % 102400 == 0) {
          LOG_INFO("/_api/files/upload: Progress: %d KB", totalUploaded / 1024);
          uploadFile.flush();
        }
      }
      
      if (final) {
        Serial.println("\n========================================");
        Serial.println("API UPLOAD FINAL FLAG");
        Serial.printf("Total: %d bytes (%d KB)\n", totalUploaded, totalUploaded / 1024);
        Serial.println("========================================");
        
        if (uploadFile) {
          uploadFile.flush();
          uploadFile.close();
          LOG_INFO("/_api/files/upload: Complete: %s (%d bytes)", uploadPath.c_str(), totalUploaded);
        }
        totalUploaded = 0;
        uploadInProgress = false;
      }
    });
}

void setupOTAEndpoint() {
  server.on("/update", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      #ifdef OTA_PASSWORD
      if (!request->hasHeader("X-OTA-Password")) {
        request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Password required\"}");
        return;
      }
      String password = request->header("X-OTA-Password");
      if (password != String(OTA_PASSWORD)) {
        LOG_WARN("Invalid OTA password attempt from %s", request->client()->remoteIP().toString().c_str());
        request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Invalid password\"}");
        return;
      }
      #endif
      
      String firmwarePath = "/firmware.bin";
      if (request->hasParam("file", true)) {
        firmwarePath = request->getParam("file", true)->value();
        if (!firmwarePath.startsWith("/")) {
          firmwarePath = "/" + firmwarePath;
        }
      }
      
      if (!SD.exists(firmwarePath)) {
        request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Firmware file not found on SD card\"}");
        LOG_ERROR("Firmware file not found: %s", firmwarePath.c_str());
        return;
      }
      
      scheduleOTAUpdate(firmwarePath);
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"OTA update will start shortly...\"}");
    }
  );
}

void setupWebUIEndpoints() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    LOG_DEBUG("Request: / from %s", request->client()->remoteIP().toString().c_str());
    
    bool indexExists = SD.exists("/index.html");
    
    if (indexExists) {
      File file = SD.open("/index.html", FILE_READ);
      if (file && !file.isDirectory()) {
        size_t fileSize = file.size();
        file.close();
        
        if (fileSize > 0) {
          if (fileSize > 100000) {
            LOG_INFO("Serving large index.html: %d bytes", fileSize);
          }
          request->send(SD, "/index.html", "text/html");
          return;
        } else {
          LOG_WARN("index.html is empty");
        }
      } else {
        if (file) file.close();
        LOG_WARN("Cannot open index.html");
      }
    }
    
    LOG_DEBUG("Serving fallback interface");
    
    uint8_t cardType = SD.cardType();
    bool sdMounted = (cardType != CARD_NONE);
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP2GO - Setup Required</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/svg+xml" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='80' font-size='80'>🚀</text></svg>">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            margin: 0;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            max-width: 600px;
            padding: 50px 40px;
            text-align: center;
        }
        .logo {
            font-size: 80px;
            margin-bottom: 20px;
        }
        h1 {
            color: #333;
            margin: 0 0 10px 0;
            font-size: 2.5em;
        }
        .subtitle {
            color: #666;
            font-size: 1.1em;
            margin-bottom: 30px;
        }
        .alert {
            background: #fff3cd;
            border: 2px solid #ffc107;
            border-radius: 10px;
            padding: 20px;
            margin: 30px 0;
            color: #856404;
        }
        .alert h3 {
            margin: 0 0 10px 0;
            font-size: 1.2em;
        }
        .status {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 15px;
            margin: 20px 0;
            font-size: 0.95em;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #dee2e6;
        }
        .status-item:last-child {
            border-bottom: none;
        }
        .btn {
            display: inline-block;
            background: #667eea;
            color: white;
            text-decoration: none;
            padding: 15px 30px;
            border-radius: 10px;
            font-weight: 500;
            margin: 10px;
            transition: transform 0.2s;
        }
        .btn:hover {
            transform: translateY(-2px);
            background: #5568d3;
        }
        .btn-secondary {
            background: #6c757d;
        }
        .btn-secondary:hover {
            background: #5a6268;
        }
        code {
            background: #f1f3f5;
            padding: 2px 8px;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">🚀</div>
        <h1>ESP2GO</h1>
        <div class="subtitle">ESP32 Storage & Control System</div>
        
        <div class="alert">
            <h3>⚠️ Setup Required</h3>
            <p>Missing <code>index.html</code> on SD card</p>
        </div>
        
        <div class="status">
            <div class="status-item">
                <span><strong>SD Card:</strong></span>
                <span>)rawliteral";
    
    if (sdMounted) {
      html += "<span style='color: #28a745'>✅ Mounted</span>";
    } else {
      html += "<span style='color: #dc3545'>❌ Not Mounted</span>";
    }
    
    html += R"rawliteral(</span>
            </div>
            <div class="status-item">
                <span><strong>IP Address:</strong></span>
                <span>)rawliteral";
    
    html += WiFi.localIP().toString();
    
    html += R"rawliteral(</span>
            </div>
            <div class="status-item">
                <span><strong>Hostname:</strong></span>
                <span>esp2go.local</span>
            </div>
        </div>
        
        <div style="margin-top: 40px;">
            <p style="color: #666; margin-bottom: 20px;">
                Please upload the web interface files to your SD card.<br>
                Visit the project documentation for setup instructions.
            </p>
            
            <a href="https://github.com/esp2go/esp2go" class="btn" target="_blank">
                📖 View Documentation
            </a>
            
            <a href="/apps/file_manager.html" class="btn btn-secondary">
                📁 File Manager
            </a>
        </div>
        
        <div style="margin-top: 30px; padding-top: 20px; border-top: 1px solid #dee2e6; color: #999; font-size: 0.85em;">
            API Endpoints available at <code>/_api/*</code>
        </div>
    </div>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    LOG_DEBUG("Request: /list from %s", request->client()->remoteIP().toString().c_str());
    
    if (SD.cardType() == CARD_NONE) {
      LOG_ERROR("/list: SD card not mounted");
      request->send(500, "application/json", "{\"error\":\"SD card not mounted\",\"files\":[]}");
      return;
    }
    
    File root = SD.open("/");
    if (!root) {
      LOG_ERROR("/list: Failed to open root directory");
      request->send(500, "application/json", "{\"error\":\"Failed to open directory\",\"files\":[]}");
      return;
    }
    
    if (!root.isDirectory()) {
      root.close();
      LOG_ERROR("/list: Root is not a directory");
      request->send(500, "application/json", "{\"error\":\"Root is not a directory\",\"files\":[]}");
      return;
    }
    
    JsonDocument doc;
    JsonArray files = doc["files"].to<JsonArray>();
    int fileCount = 0;
    listDir(root, "", files, 0, fileCount);
    root.close();
    
    String response;
    serializeJson(doc, response);
    LOG_DEBUG("/list: Returning %d files", files.size());
    request->send(200, "application/json", response);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      LOG_WARN("/download: Missing path parameter");
      request->send(400, "text/plain", "Path parameter missing");
      return;
    }
    
    String path = request->getParam("path")->value();
    LOG_INFO("/download: Request for %s from %s", path.c_str(), request->client()->remoteIP().toString().c_str());
    
    if (path.length() == 0 || path.indexOf("..") >= 0) {
      LOG_WARN("/download: Invalid path: %s", path.c_str());
      request->send(400, "text/plain", "Invalid path");
      return;
    }
    
    if (!SD.exists(path)) {
      LOG_WARN("/download: File not found: %s", path.c_str());
      request->send(404, "text/plain", "File not found: " + path);
      return;
    }
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
      LOG_ERROR("/download: Failed to open file: %s", path.c_str());
      request->send(500, "text/plain", "Failed to open file");
      return;
    }
    
    if (file.isDirectory()) {
      file.close();
      LOG_WARN("/download: Cannot download directory: %s", path.c_str());
      request->send(400, "text/plain", "Cannot download directory");
      return;
    }
    
    file.close();
    LOG_INFO("/download: Serving %s", path.c_str());
    request->send(SD, path, String(), true);
  });

  server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      LOG_WARN("/delete: Missing path parameter");
      request->send(400, "text/plain", "Path parameter missing");
      return;
    }
    
    String path = request->getParam("path")->value();
    LOG_INFO("/delete: Request for %s from %s", path.c_str(), request->client()->remoteIP().toString().c_str());
    
    if (path.length() == 0 || path.indexOf("..") >= 0) {
      LOG_WARN("/delete: Invalid path: %s", path.c_str());
      request->send(400, "text/plain", "Invalid path");
      return;
    }
    
    if (path == "/" || path == "/index.html") {
      LOG_WARN("/delete: Attempted to delete protected file: %s", path.c_str());
      request->send(403, "text/plain", "Cannot delete this file");
      return;
    }
    
    if (!SD.exists(path)) {
      LOG_WARN("/delete: File not found: %s", path.c_str());
      request->send(404, "text/plain", "File not found");
      return;
    }
    
    if (SD.remove(path)) {
      LOG_INFO("/delete: Successfully deleted: %s", path.c_str());
      request->send(200, "text/plain", "Deleted successfully");
    } else {
      LOG_ERROR("/delete: Failed to delete: %s", path.c_str());
      request->send(500, "text/plain", "Delete failed - file may be in use");
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    LOG_DEBUG("404 handler: %s from %s", path.c_str(), request->client()->remoteIP().toString().c_str());
    
    if (path.indexOf("..") >= 0) {
      LOG_WARN("404: Invalid path with ..: %s", path.c_str());
      request->send(400, "text/plain", "Invalid path");
      return;
    }
    
    if (!SD.exists(path)) {
      LOG_WARN("404: File not found: %s", path.c_str());
      request->send(404, "text/plain", "File not found: " + path);
      return;
    }
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
      LOG_ERROR("404: Cannot open file: %s", path.c_str());
      request->send(500, "text/plain", "Cannot open file");
      return;
    }
    
    if (file.isDirectory()) {
      file.close();
      LOG_WARN("404: Cannot serve directory: %s", path.c_str());
      request->send(400, "text/plain", "Cannot serve directory");
      return;
    }
    
    file.close();
    
    String contentType = "application/octet-stream";
    if (path.endsWith(".html") || path.endsWith(".htm")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".json")) contentType = "application/json";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
    else if (path.endsWith(".gif")) contentType = "image/gif";
    else if (path.endsWith(".svg")) contentType = "image/svg+xml";
    else if (path.endsWith(".ico")) contentType = "image/x-icon";
    else if (path.endsWith(".txt")) contentType = "text/plain";
    else if (path.endsWith(".pdf")) contentType = "application/pdf";
    else if (path.endsWith(".xml")) contentType = "text/xml";
    else if (path.endsWith(".zip")) contentType = "application/zip";
    else if (path.endsWith(".mp3")) contentType = "audio/mpeg";
    else if (path.endsWith(".mp4")) contentType = "video/mp4";
    else if (path.endsWith(".woff")) contentType = "font/woff";
    else if (path.endsWith(".woff2")) contentType = "font/woff2";
    else if (path.endsWith(".ttf")) contentType = "font/ttf";
    
    LOG_INFO("Serving static file: %s (%s)", path.c_str(), contentType.c_str());
    request->send(SD, path, contentType);
  });
}

void setupWebServer() {
  LOG_INFO("Setting up web server...");
  
  setupAPIEndpoints();
  setupGPIOEndpoints();
  setupFileEndpoints();
  setupOTAEndpoint();
  setupWebUIEndpoints();
  
  server.begin();
  LOG_INFO("Web server started successfully");
  
  Serial.println("\n======> THIS IS FROM OTA VERSION 0 <======\n");
}

