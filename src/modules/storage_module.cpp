#include "storage_module.h"
#include "../config.h"
#include <SD.h>

// Helper: Recursive directory deletion
static bool deleteRecursive(const String& path) {
  File file = SD.open(path);
  if (!file) return false;
  
  if (!file.isDirectory()) {
    file.close();
    return SD.remove(path);
  }
  
  file.rewindDirectory();
  File entry = file.openNextFile();
  while (entry) {
    String entryPath = String(path);
    if (!entryPath.endsWith("/")) entryPath += "/";
    entryPath += entry.name();
    
    if (entry.isDirectory()) {
      entry.close();
      if (!deleteRecursive(entryPath)) {
        file.close();
        return false;
      }
    } else {
      entry.close();
      if (!SD.remove(entryPath)) {
        file.close();
        return false;
      }
    }
    entry = file.openNextFile();
  }
  
  file.close();
  return SD.rmdir(path);
}

// Helper: Create directory path recursively
static void createDirectoryPath(const String& path) {
  String currentPath = "";
  int start = (path.startsWith("/")) ? 1 : 0;
  int slashIndex = path.indexOf('/', start);
  
  while (slashIndex > 0) {
    currentPath = path.substring(0, slashIndex);
    if (!SD.exists(currentPath)) {
      SD.mkdir(currentPath);
    }
    slashIndex = path.indexOf('/', slashIndex + 1);
  }
  
  if (!SD.exists(path)) {
    SD.mkdir(path);
  }
}

bool StorageModule::init() {
    logInfo("Initializing...");
    
    if (!SD.begin()) {
        logError("SD Card initialization failed");
        return false;
    }
    
    totalSpace = SD.totalBytes();
    usedSpace = SD.usedBytes();
    freeSpace = totalSpace - usedSpace;
    
    logInfo("SD Card: %llu MB total, %llu MB used, %llu MB free", 
            totalSpace / 1024 / 1024, 
            usedSpace / 1024 / 1024, 
            freeSpace / 1024 / 1024);
    
    logInfo("Initialized successfully");
    return true;
}

void StorageModule::update() {
    // Update storage stats every 30 seconds
    if (millis() - lastStatsUpdate > 30000) {
        lastStatsUpdate = millis();
        updateStats();
    }
}

void StorageModule::getStatus(JsonObject& obj) const {
    ModuleBase::getStatus(obj);
    obj["total_bytes"] = totalSpace;
    obj["used_bytes"] = usedSpace;
    obj["free_bytes"] = freeSpace;
    obj["total_mb"] = totalSpace / 1024 / 1024;
    obj["used_mb"] = usedSpace / 1024 / 1024;
    obj["free_mb"] = freeSpace / 1024 / 1024;
}

void StorageModule::updateStats() {
    totalSpace = SD.totalBytes();
    usedSpace = SD.usedBytes();
    freeSpace = totalSpace - usedSpace;
}

void StorageModule::registerAPI(AsyncWebServer& server) {
    // Storage info
    server.on("/_api/files/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateStats();
        
        JsonDocument doc;
        doc["total"] = totalSpace;
        doc["used"] = usedSpace;
        doc["free"] = freeSpace;
        doc["total_mb"] = totalSpace / 1024 / 1024;
        doc["used_mb"] = usedSpace / 1024 / 1024;
        doc["free_mb"] = freeSpace / 1024 / 1024;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // List files
    server.on("/_api/files/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
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
    
    // File info
    server.on("/_api/files/file", HTTP_GET, [this](AsyncWebServerRequest *request) {
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
    
    // Create directory
    server.on("/_api/files/mkdir", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error || !doc["path"].is<const char*>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid path parameter\"}");
                return;
            }
            
            String path = doc["path"].as<String>();
            
            if (path.length() == 0 || path.indexOf("..") >= 0) {
                request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
                return;
            }
            
            if (SD.exists(path)) {
                request->send(409, "application/json", "{\"error\":\"Path already exists\"}");
                return;
            }
            
            createDirectoryPath(path);
            
            if (SD.exists(path)) {
                logInfo("Created directory: %s", path.c_str());
                request->send(200, "application/json", "{\"status\":\"created\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Failed to create directory\"}");
            }
        }
    );
    
    // Move/rename
    server.on("/_api/files/move", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error || !doc["source"].is<const char*>() || !doc["destination"].is<const char*>()) {
                request->send(400, "application/json", "{\"error\":\"Missing source or destination\"}");
                return;
            }
            
            String source = doc["source"].as<String>();
            String destination = doc["destination"].as<String>();
            
            if (source.length() == 0 || destination.length() == 0 || 
                source.indexOf("..") >= 0 || destination.indexOf("..") >= 0) {
                request->send(400, "application/json", "{\"error\":\"Invalid paths\"}");
                return;
            }
            
            if (source == "/" || source == PATH_INDEX) {
                request->send(403, "application/json", "{\"error\":\"Cannot move protected file\"}");
                return;
            }
            
            if (!SD.exists(source)) {
                request->send(404, "application/json", "{\"error\":\"Source not found\"}");
                return;
            }
            
            if (SD.exists(destination)) {
                request->send(409, "application/json", "{\"error\":\"Destination already exists\"}");
                return;
            }
            
            int lastSlash = destination.lastIndexOf('/');
            if (lastSlash > 0) {
                String destDir = destination.substring(0, lastSlash);
                if (!SD.exists(destDir)) {
                    createDirectoryPath(destDir);
                }
            }
            
            if (SD.rename(source, destination)) {
                logInfo("Moved: %s -> %s", source.c_str(), destination.c_str());
                request->send(200, "application/json", "{\"status\":\"moved\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Move failed\"}");
            }
        }
    );
    
    // Delete
    server.on("/_api/files/delete", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "application/json", "{\"error\":\"Missing path\"}");
            return;
        }
        
        String path = request->getParam("path")->value();
        
        if (path.length() == 0 || path.indexOf("..") >= 0) {
            request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
            return;
        }
        
        if (path == "/" || path == PATH_INDEX) {
            request->send(403, "application/json", "{\"error\":\"Cannot delete protected file\"}");
            return;
        }
        
        if (!SD.exists(path)) {
            request->send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
        }
        
        if (deleteRecursive(path)) {
            logInfo("Deleted: %s", path.c_str());
            request->send(200, "application/json", "{\"status\":\"deleted\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"Failed to delete\"}");
        }
    });
    
    // Upload
    server.on("/_api/files/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            static File uploadFile;
            static String uploadFilePath;
            static size_t bytesWritten = 0;
            static AsyncWebServerRequest* currentRequest = nullptr;
            
            auto sendError = [](AsyncWebServerRequest* req, const String& errorMsg, const String& path) {
                if (!req) return;
                JsonDocument doc;
                doc["status"] = "error";
                doc["error"] = errorMsg;
                doc["path"] = path;
                String response;
                serializeJson(doc, response);
                req->send(500, "application/json", response);
            };
            
            auto cleanup = [&]() {
                if (uploadFile) {
                    uploadFile.close();
                    uploadFile = File();
                }
                uploadFilePath = "";
                bytesWritten = 0;
                currentRequest = nullptr;
            };
            
            if (!index) {
                cleanup();
                currentRequest = request;
                
                String path = request->hasParam("path") ? request->getParam("path")->value() : "/";
                if (!path.endsWith("/")) path += "/";
                
                uploadFilePath = path + filename;
                
                int lastSlash = uploadFilePath.lastIndexOf('/');
                if (lastSlash > 0) {
                    String dir = uploadFilePath.substring(0, lastSlash);
                    if (!SD.exists(dir)) {
                        createDirectoryPath(dir);
                    }
                }
                
                logInfo("Upload start: %s", uploadFilePath.c_str());
                uploadFile = SD.open(uploadFilePath, FILE_WRITE);
                if (!uploadFile) {
                    logError("Failed to open file for writing: %s", uploadFilePath.c_str());
                    sendError(currentRequest, "Failed to open file for writing", uploadFilePath);
                    cleanup();
                    return;
                }
            }
            
            if (len > 0) {
                if (!uploadFile) {
                    logError("Upload file not open during write");
                    sendError(currentRequest, "File was closed unexpectedly", uploadFilePath);
                    cleanup();
                    return;
                }
                
                size_t written = uploadFile.write(data, len);
                bytesWritten += written;
                
                if (written != len) {
                    logWarn("Write mismatch: expected %d, wrote %d", len, written);
                }
            }
            
            if (final) {
                if (!uploadFile) {
                    logError("Upload file not open on final");
                    sendError(currentRequest, "File was not open", uploadFilePath);
                    cleanup();
                    return;
                }
                
                uploadFile.flush();
                uploadFile.close();
                
                File verifyFile = SD.open(uploadFilePath, FILE_READ);
                size_t fileSizeOnDisk = 0;
                if (verifyFile) {
                    fileSizeOnDisk = verifyFile.size();
                    verifyFile.close();
                } else {
                    logWarn("Could not verify uploaded file size");
                }
                
                logInfo("Upload complete: %s (written: %d bytes, on disk: %d bytes)", 
                        uploadFilePath.c_str(), bytesWritten, fileSizeOnDisk);
                
                if (currentRequest) {
                    JsonDocument doc;
                    doc["status"] = "uploaded";
                    doc["path"] = uploadFilePath;
                    doc["bytes_written"] = bytesWritten;
                    doc["file_size"] = fileSizeOnDisk;
                    doc["success"] = (fileSizeOnDisk == bytesWritten);
                    
                    if (fileSizeOnDisk != bytesWritten) {
                        doc["warning"] = "File size mismatch detected";
                        logWarn("Size mismatch: written %d, on disk %d", bytesWritten, fileSizeOnDisk);
                    }
                    
                    String response;
                    serializeJson(doc, response);
                    currentRequest->send(200, "application/json", response);
                }
                
                cleanup();
            }
        }
    );
    
    // Download
    server.on("/_api/files/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "application/json", "{\"error\":\"Missing path\"}");
            return;
        }
        
        String path = request->getParam("path")->value();
        
        if (path.indexOf("..") >= 0) {
            request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
            return;
        }
        
        if (!SD.exists(path)) {
            request->send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
        }
        
        File file = SD.open(path);
        if (file.isDirectory()) {
            file.close();
            request->send(400, "application/json", "{\"error\":\"Cannot download directory\"}");
            return;
        }
        
        request->send(SD, path, "application/octet-stream");
    });
}

