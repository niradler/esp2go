# SD Card Files Guide

This directory contains the web interface files for ESP2GO. Files are organized into folders for better structure.

## Folder Structure

```
sd_card/
├── index.html              # Dashboard (main page)
├── apps/                   # Application HTML files
│   ├── button_app.html
│   ├── file_manager.html
│   ├── gpio_app.html
│   ├── led_app.html
│   └── mic_app.html
├── docs/                   # Documentation files
│   ├── api_docs.html       # Swagger UI
│   └── openapi.yaml        # OpenAPI specification
└── os/                     # OS-level configuration files
    ├── ota_update.html
    └── wifi_config.json
```

## Required Files

### 📄 `/os/wifi_config.json`

WiFi configuration with saved networks and AP settings. This file should be placed in the `/os/` folder on the SD card.

```json
{
  "networks": [
    {
      "ssid": "YourWiFi",
      "password": "YourPassword",
      "priority": 1
    }
  ],
  "ap_ssid": "ESP2GO",
  "ap_password": "12345678"
}
```

## Web Applications

### 📱 `index.html` - Dashboard

Main control panel with dynamic app discovery:
- Automatically discovers apps in `/apps/` folder
- Real-time system status
- Quick access to all features
- Mobile-optimized UI

**Access:** `http://<ESP_IP>/` or `http://esp2go.local/`

### 📁 `/apps/file_manager.html` - File Manager

Browse and manage SD card files:
- Navigate folder structure
- Upload files
- Download files
- Delete files

**Access:** `http://<ESP_IP>/apps/file_manager.html`
