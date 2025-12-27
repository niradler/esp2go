#include "server.h"
#include "config.h"
#include "hardware.h"
#include "storage.h"
#include "ota.h"
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
      LOG_WARN("/_api/files/delete: Missing path parameter");
      request->send(400, "application/json", "{\"error\":\"Missing path\"}");
      return;
    }
    
    String path = request->getParam("path")->value();
    LOG_INFO("/_api/files/delete: Request for %s from %s", path.c_str(), request->client()->remoteIP().toString().c_str());
    
    if (path.length() == 0 || path.indexOf("..") >= 0) {
      LOG_WARN("/_api/files/delete: Invalid path: %s", path.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
      return;
    }
    
    if (path == "/" || path == "/index.html") {
      LOG_WARN("/_api/files/delete: Attempted to delete protected file: %s", path.c_str());
      request->send(403, "application/json", "{\"error\":\"Cannot delete protected file\"}");
      return;
    }
    
    if (!SD.exists(path)) {
      LOG_WARN("/_api/files/delete: File not found: %s", path.c_str());
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }
    
    File file = SD.open(path);
    bool isDir = file.isDirectory();
    file.close();
    
    if (isDir) {
      if (SD.rmdir(path)) {
        LOG_INFO("/_api/files/delete: Successfully deleted directory: %s", path.c_str());
        request->send(200, "application/json", "{\"status\":\"deleted\"}");
      } else {
        LOG_ERROR("/_api/files/delete: Failed to delete directory (may not be empty): %s", path.c_str());
        request->send(500, "application/json", "{\"error\":\"Directory not empty or in use\"}");
      }
    } else {
      if (SD.remove(path)) {
        LOG_INFO("/_api/files/delete: Successfully deleted file: %s", path.c_str());
        request->send(200, "application/json", "{\"status\":\"deleted\"}");
      } else {
        LOG_ERROR("/_api/files/delete: Failed to delete file: %s", path.c_str());
        request->send(500, "application/json", "{\"error\":\"Delete failed - file may be in use\"}");
      }
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
  
  server.on("/_api/files/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      LOG_WARN("/_api/files/download: Missing path parameter");
      request->send(400, "application/json", "{\"error\":\"Missing path\"}");
      return;
    }
    
    String path = request->getParam("path")->value();
    LOG_INFO("/_api/files/download: Request for %s from %s", path.c_str(), request->client()->remoteIP().toString().c_str());
    
    if (path.length() == 0 || path.indexOf("..") >= 0) {
      LOG_WARN("/_api/files/download: Invalid path: %s", path.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
      return;
    }
    
    if (!SD.exists(path)) {
      LOG_WARN("/_api/files/download: File not found: %s", path.c_str());
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
      LOG_ERROR("/_api/files/download: Failed to open file: %s", path.c_str());
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }
    
    if (file.isDirectory()) {
      file.close();
      LOG_WARN("/_api/files/download: Cannot download directory: %s", path.c_str());
      request->send(400, "application/json", "{\"error\":\"Cannot download directory\"}");
      return;
    }
    
    file.close();
    LOG_INFO("/_api/files/download: Serving %s", path.c_str());
    request->send(SD, path, String(), true);
  });
}

void setupOTAEndpoint() {
  server.on("/_api/ota/update", HTTP_POST, 
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
      
      String firmwarePath = PATH_FIRMWARE_DEFAULT;
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
    
    bool indexExists = SD.exists(PATH_INDEX);
    
    if (indexExists) {
      File file = SD.open(PATH_INDEX, FILE_READ);
      if (file && !file.isDirectory()) {
        size_t fileSize = file.size();
        file.close();
        
        if (fileSize > 0) {
          if (fileSize > 100000) {
            LOG_INFO("Serving large index.html: %d bytes", fileSize);
          }
          request->send(SD, PATH_INDEX, "text/html");
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
    uint64_t cardSize = sdMounted ? SD.cardSize() / (1024 * 1024) : 0;
    uint64_t usedBytes = sdMounted ? SD.usedBytes() / (1024 * 1024) : 0;
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP2GO Manager - Fallback</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
</head>
<body class="bg-light">
    <div class="container my-4">
        <div class="card shadow">
            <div class="card-header bg-primary text-white">
                <h1 class="h3 mb-0">ESP2GO Manager</h1>
                <small>Fallback Interface</small>
            </div>
            <div class="card-body">
                <div class="alert alert-warning">
                    <strong>⚠️ Using Fallback Interface</strong><br>
                    No custom index.html found on SD card. Upload one to customize!
                </div>
                
                <h5 class="mt-4">📊 System Status</h5>
                <table class="table table-sm">
                    <tbody>
                        <tr>
                            <td class="fw-bold">SD Card Status:</td>
                            <td>)rawliteral";
    
    if (sdMounted) {
      html += "<span class='text-success'>✅ Mounted</span>";
    } else {
      html += "<span class='text-danger'>❌ Not Mounted</span>";
    }
    
    html += R"rawliteral(</td>
                        </tr>
                        <tr>
                            <td class="fw-bold">SD Card Size:</td>
                            <td>)rawliteral";
    
    html += String(cardSize) + " MB";
    
    html += R"rawliteral(</td>
                        </tr>
                        <tr>
                            <td class="fw-bold">Used Space:</td>
                            <td>)rawliteral";
    
    html += String(usedBytes) + " MB";
    
    html += R"rawliteral(</td>
                        </tr>
                        <tr>
                            <td class="fw-bold">index.html exists:</td>
                            <td>)rawliteral";
    
    if (SD.exists("/index.html")) {
      html += "<span class='text-success'>✅ Yes</span>";
    } else {
      html += "<span class='text-warning'>⚠️ No (using fallback)</span>";
    }
    
    html += R"rawliteral(</td>
                        </tr>
                    </tbody>
                </table>
                
                <h5 class="mt-4">📤 Upload File</h5>
                <form id="uploadForm" enctype="multipart/form-data" class="mb-3">
                    <div class="input-group">
                        <input type="file" class="form-control" id="fileInput" name="file">
                        <button type="submit" class="btn btn-primary">Upload</button>
                    </div>
                </form>
                
                <h5 class="mt-4">📁 Files</h5>
                <div id="fileList" class="list-group"></div>
            </div>
        </div>
    </div>
    <script>
        function loadFiles() {
            fetch('/list')
                .then(response => response.json())
                .then(data => {
                    const fileList = document.getElementById('fileList');
                    if (data.files.length === 0) {
                        fileList.innerHTML = '<div class="alert alert-info">No files on SD card</div>';
                        return;
                    }
                    fileList.innerHTML = '';
                    data.files.forEach(file => {
                        const div = document.createElement('div');
                        div.className = 'list-group-item d-flex justify-content-between align-items-center';
                        
                        const info = document.createElement('div');
                        const icon = file.isDir ? '📁' : '📄';
                        info.innerHTML = '<strong>' + icon + ' ' + escapeHtml(file.name) + '</strong> <small class="text-muted">(' + formatBytes(file.size) + ')</small>';
                        
                        const btnGroup = document.createElement('div');
                        btnGroup.className = 'btn-group btn-group-sm';
                        
                        if (!file.isDir) {
                            const downloadBtn = document.createElement('button');
                            downloadBtn.className = 'btn btn-outline-primary';
                            downloadBtn.textContent = 'Download';
                            downloadBtn.onclick = function() { downloadFile(file.path); };
                            btnGroup.appendChild(downloadBtn);
                        }
                        
                        const deleteBtn = document.createElement('button');
                        deleteBtn.className = 'btn btn-outline-danger';
                        deleteBtn.textContent = 'Delete';
                        deleteBtn.onclick = function() { deleteFile(file.path); };
                        btnGroup.appendChild(deleteBtn);
                        
                        div.appendChild(info);
                        div.appendChild(btnGroup);
                        fileList.appendChild(div);
                    });
                });
        }
        
        function formatBytes(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
        }
        
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        function downloadFile(path) {
            window.location.href = '/download?path=' + encodeURIComponent(path);
        }
        
        function deleteFile(path) {
            if (confirm('Delete ' + path + '?')) {
                fetch('/delete?path=' + encodeURIComponent(path), {method: 'DELETE'})
                    .then(() => loadFiles());
            }
        }
        
        document.getElementById('uploadForm').onsubmit = function(e) {
            e.preventDefault();
            const formData = new FormData();
            formData.append('file', document.getElementById('fileInput').files[0]);
            fetch('/upload', {
                method: 'POST',
                body: formData
            }).then(() => {
                loadFiles();
                document.getElementById('fileInput').value = '';
                alert('Upload complete! Refresh the page if you uploaded index.html');
            });
        };
        
        loadFiles();
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
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

