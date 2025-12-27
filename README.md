# ESP2GO 🚀

**A web-based operating system for ESP32 that lets you build cool electronic projects entirely from your browser.**

ESP2GO transforms your ESP32 into a powerful platform where you can create interactive hardware projects using simple HTML/JavaScript. Upload apps to the SD card, access them from any device on your network, and control hardware through a clean REST API - no firmware changes needed!

## ✨ What is ESP2GO?

ESP2GO is a **framework and runtime environment** for building browser-based electronic projects. Think of it as an OS for your ESP32:

- 📱 **Web-Based Apps**: Write HTML/JS apps and upload them to the SD card
- 🔌 **Hardware API**: Control GPIO, LEDs, sensors via REST API
- 🌐 **Network Access**: Access via WiFi (mDNS: `esp2go.local`)
- 💾 **SD Card Storage**: All apps and data stored on removable SD card
- 🔄 **Live Updates**: Upload new apps without reflashing firmware
- 📚 **Example Apps**: LED controller, GPIO control, audio recording, and more

### Perfect For

- 🎓 **Learning**: Learn web development with physical hardware feedback
- 🛠️ **Prototyping**: Quickly build and test IoT ideas
- 🎨 **Creative Projects**: Interactive art, home automation, robotics
- 📊 **Data Logging**: Monitor sensors and store data to SD card
- 🎮 **Control Panels**: Custom dashboards for your projects

## 🚀 Quick Start

### 1. Hardware Setup

**Required:**

- M5Stack AtomS3U (or compatible ESP32-S3 board)
- SD Card (FAT32 formatted)
- USB-C cable for programming

**SD Card Wiring:**

```
SD Card Pin → ESP32 Pin
MISO       → GPIO 14
MOSI       → GPIO 17
SCK        → GPIO 42
CS         → GPIO 40
```

### 2. Flash Firmware

```bash
# Clone repository
git clone https://github.com/esp2go/esp2go
cd esp2go

# Configure WiFi (optional - can be done via SD card too)
cp .env.example .env
# Edit .env with your WiFi credentials

# Build and flash
pio run -t upload -t monitor
```

### 3. Upload Apps to SD Card

Copy the entire `sd_card/` folder contents to your SD card root:

```
SD Card Root/
├── index.html          # Dashboard (loads automatically)
├── apps/               # Your applications
├── docs/               # API documentation
└── os/                 # System configuration
```

### 4. Access Your Device

- **mDNS**: `http://esp2go.local/` (recommended)
- **IP Address**: Check serial monitor for assigned IP
- **AP Mode**: `http://192.168.4.1/` (if WiFi fails)

## 🎯 Building Your First App

ESP2GO apps are just HTML files that call the REST API. Here's a simple example:

```html
<!DOCTYPE html>
<html>
  <head>
    <title>My First App</title>
  </head>
  <body>
    <h1>LED Controller</h1>
    <button onclick="setLED(255, 0, 0)">Red</button>
    <button onclick="setLED(0, 255, 0)">Green</button>
    <button onclick="setLED(0, 0, 255)">Blue</button>

    <script>
      async function setLED(r, g, b) {
        await fetch("/_api/led/set", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ r, g, b }),
        });
      }
    </script>
  </body>
</html>
```

Save this as `/apps/my_app.html` on the SD card and access it at `http://esp2go.local/apps/my_app.html`

## 📦 SD Card Structure

```
sd_card/
├── index.html                  # Dashboard - loads on root access
│                               # Dynamically discovers apps in /apps/
│
├── apps/                       # Your applications go here
│   ├── file_manager.html      # Browse/upload/delete files
│   ├── led_app.html           # RGB LED color picker
│   ├── mic_app.html           # Audio level monitor + recording
│   ├── button_app.html        # Button state monitor
│   └── gpio_app.html          # GPIO pin control (read/write/PWM)
│
├── docs/                       # API documentation
│   ├── api_docs.html          # Swagger UI (interactive API testing)
│   └── openapi.yaml           # OpenAPI specification
│
└── os/                         # System files
    ├── ota_update.html        # Firmware update interface
    └── wifi_config.json       # WiFi network configuration
```

### Dynamic Dashboard

The `index.html` dashboard automatically discovers and displays all apps in the `/apps/` folder. Just drop a new HTML file there and it appears on the dashboard!

## 🔌 Hardware & Pinout

### M5Stack AtomS3U Built-in Components

| Component  | GPIO  | Status | Usage                          |
| ---------- | ----- | ------ | ------------------------------ |
| RGB LED    | 35    | ✅     | Onboard WS2812 RGB LED         |
| Button     | 41    | ✅     | Built-in push button           |
| Microphone | 38/39 | ✅     | PDM mic (data/clock)           |
| IR TX      | 4     | ⏳     | Infrared transmitter (planned) |

### SD Card Interface (SPI)

| Pin  | GPIO | Usage            |
| ---- | ---- | ---------------- |
| MISO | 14   | SD card data out |
| MOSI | 17   | SD card data in  |
| SCK  | 42   | SPI clock        |
| CS   | 40   | Chip select      |

**Note:** GPIO 14, 17, 42, 40 are dedicated to SD card and not available for general use.

### Available GPIO Pins

**Free for your projects:**

```
GPIO: 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 18, 21
```

**Reserved/In Use:**

- GPIO 14, 17, 42, 40 → SD Card
- GPIO 35 → RGB LED
- GPIO 38, 39 → Microphone
- GPIO 41 → Button
- GPIO 4 → IR Emitter

### Expansion Interfaces

- **Grove Port (PORT.A)**: I2C expansion (SCL: GPIO 1, SDA: GPIO 2)
- **6-Pin Header**: Exposes SD card pins for debugging

## 🛠️ REST API

All hardware control is done via REST API at `/_api/*`. Full documentation available at `http://esp2go.local/docs/api_docs.html`

### Quick Reference

**System**

```bash
GET /_api/system/info        # Chip info, memory, uptime
GET /_api/wifi/status        # WiFi connection status
```

**LED Control**

```bash
POST /_api/led/set
Body: {"r": 255, "g": 0, "b": 0}
```

**Button**

```bash
GET /_api/button/status      # Returns {"pressed": true/false}
```

**Microphone**

```bash
GET  /_api/mic/level         # Current audio level
POST /_api/mic/record/start  # Start recording to SD
POST /_api/mic/record/stop   # Stop recording
```

**GPIO Control**

```bash
POST /_api/gpio/mode         # Set pin mode
     Body: {"pin": 5, "mode": "OUTPUT"}

POST /_api/gpio/write        # Write value
     Body: {"pin": 5, "value": 1}

GET  /_api/gpio/read?pin=5   # Read digital value
GET  /_api/gpio/analog?pin=1 # Read analog value
GET  /_api/gpio/pins         # List available pins
```

**File Management**

```bash
GET    /_api/files/list?path=/     # List directory
GET    /_api/files/download?path=/ # Download file
POST   /_api/files/upload          # Upload file
DELETE /_api/files/delete?path=/   # Delete file
```

**OTA Update**

```bash
POST /_api/ota/update
Header: X-OTA-Password: your_password
Body: file=/firmware.bin
```

## 📱 Example Apps

### LED Controller (`/apps/led_app.html`)

Interactive RGB color picker with sliders and preset colors. Control the onboard WS2812 LED in real-time.

### File Manager (`/apps/file_manager.html`)

Complete file browser with:

- Multi-file upload (sequential, won't overwhelm ESP32)
- Multi-file delete (selection mode)
- Download files
- Navigate folders
- Drag-and-drop support

### GPIO Control (`/apps/gpio_app.html`)

Universal GPIO interface:

- Set pin modes (INPUT/OUTPUT/PULLUP/PULLDOWN)
- Read/write digital values
- Read analog values (ADC)
- PWM control (planned)
- Pin status overview

### Microphone Monitor (`/apps/mic_app.html`)

Real-time audio visualization:

- Live audio level meter
- Record audio to SD card as WAV
- Playback recordings
- PDM microphone support

### Button Monitor (`/apps/button_app.html`)

Track button presses with event logging and debouncing.

### API Documentation (`/docs/api_docs.html`)

Interactive Swagger UI for testing all API endpoints directly from your browser.

## ⚙️ Configuration

### WiFi Setup

**Option 1: SD Card (Recommended)**

Create `/os/wifi_config.json` on your SD card:

```json
{
  "networks": [
    {
      "ssid": "HomeNetwork",
      "password": "password123",
      "priority": 1
    },
    {
      "ssid": "WorkNetwork",
      "password": "workpass456",
      "priority": 2
    }
  ],
  "ap_ssid": "ESP2GO",
  "ap_password": "12345678"
}
```

**Option 2: Build-Time (.env file)**

```ini
WIFI_SSID = YourWiFiName
WIFI_PASSWORD = YourPassword
AP_SSID = ESP2GO
AP_PASSWORD = 12345678
MDNS_HOSTNAME = esp2go
```

**Fallback**: If WiFi fails, ESP2GO automatically starts an access point.

### OTA Updates

1. Build new firmware: `pio run`
2. Upload `firmware.bin` to SD card via file manager
3. Navigate to `/os/ota_update.html`
4. Select firmware file and update
5. Device automatically reboots with new firmware

## 🎓 Learn More

### How It Works

1. **Boot**: ESP2GO firmware initializes hardware and SD card
2. **WiFi**: Connects to configured network (or starts AP)
3. **mDNS**: Advertises as `esp2go.local` on network
4. **Web Server**: Serves files from SD card at `/`
5. **API Server**: Handles hardware control at `/_api/*`
6. **Dashboard**: Dynamic discovery of apps in `/apps/`

### Creating Complex Apps

ESP2GO apps can:

- Use any JavaScript framework (React, Vue, vanilla JS)
- Make fetch() calls to the API
- Store data on SD card
- Use WebSockets (planned)
- Include images, CSS, fonts
- Work offline (PWA support planned)

### Best Practices

✅ **DO:**

- Keep apps small and focused
- Use async/await for API calls
- Handle errors gracefully
- Test on mobile devices
- Use the file manager for development

❌ **DON'T:**

- Block the main thread with heavy computation
- Upload huge files (>5MB) without chunking
- Hardcode IP addresses (use relative URLs)
- Delete system files in `/os/`

## 🔧 Development Tools

### Monitor Serial Output

```bash
pio device monitor
# or combined
pio run -t upload -t monitor
```

### Build Commands

```bash
pio run              # Build only
pio run -t upload    # Build and upload
pio run -t clean     # Clean build
pio pkg update       # Update dependencies
```

### Debugging

- Use Chrome DevTools for web debugging
- Serial monitor for ESP32 logs
- Check `/docs/api_docs.html` for API testing

## 🆘 Troubleshooting

### No SD Card Detected

- Check wiring (MISO/MOSI/SCK/CS)
- Format as FAT32
- Try different SD card
- Check serial monitor for errors

### Can't Access Device

- Verify WiFi connection (check serial monitor for IP)
- Try mDNS: `http://esp2go.local/`
- If all fails, use AP mode: `http://192.168.4.1/`

### Upload Fails

- Check USB connection
- Find port: `pio device list`
- Hold BOOT button during upload
- Erase flash: `pio run -t erase`

### App Not Appearing on Dashboard

- Ensure file is in `/apps/` folder
- Check filename ends with `.html`
- Refresh browser
- Check serial monitor for SD card errors

## 📚 Resources

- **API Documentation**: `http://esp2go.local/docs/api_docs.html`
- **OpenAPI Spec**: `/sd_card/docs/openapi.yaml`
- **Example Apps**: All apps in `/sd_card/apps/` are open source
- **GitHub**: [github.com/esp2go/esp2go](https://github.com/esp2go/esp2go)

## 🤝 Contributing

Contributions welcome! Areas of interest:

- New example apps
- API endpoint improvements
- Documentation
- Hardware support
- Bug fixes

## 📄 License

MIT License - Free for personal and commercial use.

## 🌟 Credits

Built with:

- ESP32 AsyncWebServer
- ArduinoJson
- M5Unified (for AtomS3U support)
- Bootstrap 5 (UI framework)

---

**Made with ❤️ for makers, hobbyists, and hardware enthusiasts**

Start building cool electronic projects today! 🚀
