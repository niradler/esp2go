#include "web_server_module.h"
#include "../config.h"
#include <SD.h>

bool WebServerModule::init() {
    logInfo("Initializing...");
    logInfo("Initialized successfully");
    return true;
}

void WebServerModule::update() {
}

String WebServerModule::getContentType(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".gif")) return "image/gif";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".xml")) return "text/xml";
    if (path.endsWith(".pdf")) return "application/pdf";
    if (path.endsWith(".zip")) return "application/zip";
    if (path.endsWith(".gz")) return "application/gzip";
    return "text/plain";
}

void WebServerModule::serveFallbackIndex(AsyncWebServerRequest *request) {
    String html = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP2GO - Setup Required</title>
    <link rel="icon" type="image/png" href="/favicon.png">
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0; padding: 0; min-height: 100vh;
            display: flex; align-items: center; justify-content: center;
        }
        .container {
            background: white; border-radius: 16px; padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3); max-width: 600px; text-align: center;
        }
        h1 { color: #667eea; margin: 0 0 10px 0; font-size: 2.5em; }
        .warning { background: #fff3cd; border: 2px solid #ffc107; border-radius: 8px;
                   padding: 20px; margin: 20px 0; color: #856404; }
        .warning h2 { margin-top: 0; color: #856404; }
        .btn { display: inline-block; padding: 12px 24px; margin: 10px;
               background: #667eea; color: white; text-decoration: none;
               border-radius: 8px; font-weight: 600; transition: all 0.3s; }
        .btn:hover { background: #5568d3; transform: translateY(-2px); }
        .btn-secondary { background: #6c757d; }
        .btn-secondary:hover { background: #5a6268; }
        code { background: #f8f9fa; padding: 2px 6px; border-radius: 4px; color: #d63384; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 ESP2GO</h1>
        <p style="color: #6c757d; font-size: 1.1em;">Your ESP32 IoT Framework</p>
        
        <div class="warning">
            <h2>⚠️ Setup Required</h2>
            <p>No <code>index.html</code> found on the SD card.</p>
            <p>Please upload your web application to the SD card root.</p>
        </div>
        
        <p style="margin: 30px 0;">Get started by:</p>
        
        <div>
            <a href="/apps/file_manager.html" class="btn">📁 Open File Manager</a>
            <a href="https://github.com/your-repo/esp2go" class="btn btn-secondary" target="_blank">📖 Documentation</a>
        </div>
        
        <p style="margin-top: 40px; color: #6c757d; font-size: 0.9em;">
            Build amazing IoT projects with ESP2GO!
        </p>
    </div>
</body>
</html>)";
    
    request->send(200, "text/html", html);
}

void WebServerModule::registerAPI(AsyncWebServer& server) {
    // Serve static files from SD card
    server.serveStatic("/", SD, "/").setDefaultFile("index.html");
    
    // Handle root - custom logic for fallback
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (SD.exists(PATH_INDEX)) {
            request->send(SD, PATH_INDEX, "text/html");
        } else {
            serveFallbackIndex(request);
        }
    });
    
    // 404 handler
    server.onNotFound([this](AsyncWebServerRequest *request) {
        String path = request->url();
        
        // Try to serve from SD card
        if (SD.exists(path)) {
            String contentType = getContentType(path);
            request->send(SD, path, contentType);
        } else {
            logWarn("404: %s", path.c_str());
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        }
    });
}

