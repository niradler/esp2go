#include "hardware.h"
#include <M5Unified.h>
#include <SD.h>

// Microphone configuration (PDM)
#define MIC_DATA_PIN 39
#define MIC_CLK_PIN 38
#define MIC_SAMPLE_RATE 16000
#define MIC_BUFFER_SIZE 512

// Hardware pins
#define LED_PIN 35
#define BUTTON_PIN 41
#define SDCARD_MISO 14
#define SDCARD_MOSI 17
#define SDCARD_SCK 42
#define SDCARD_CS 40

// Microphone state
static bool micInitialized = false;
static int16_t micBuffer[MIC_BUFFER_SIZE]; // Changed to int16_t for M5Unified
static volatile int currentAudioLevel = 0;

// Recording state
static bool recording = false;
static File recordingFile;
static uint32_t recordingStartTime = 0;
static uint32_t recordingDataSize = 0;

// WAV file header structure
struct WAVHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t fileSize;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 16;
  uint16_t audioFormat = 1; // PCM
  uint16_t numChannels = 1; // Mono
  uint32_t sampleRate = MIC_SAMPLE_RATE;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize;
};

void setupMicrophone() {
  Serial.println("ℹ️  INFO: Initializing SPM1423 PDM microphone...");
  
  // Configure M5Unified microphone
  auto cfg = M5.Mic.config();
  cfg.sample_rate = MIC_SAMPLE_RATE;
  cfg.stereo = false;  // Mono
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 128;
  cfg.task_priority = 2;
  cfg.task_pinned_core = APP_CPU_NUM;
  
  M5.Mic.config(cfg);
  
  if (M5.Mic.begin()) {
    micInitialized = true;
    Serial.println("ℹ️  INFO: SPM1423 microphone initialized successfully!");
  } else {
    Serial.println("❌ ERROR: Failed to initialize microphone");
    micInitialized = false;
  }
}

int readMicrophoneLevel() {
  if (!micInitialized) {
    return 0;
  }

  // Read samples using M5Unified - directly into int16_t buffer
  if (M5.Mic.isEnabled()) {
    bool success = M5.Mic.record(micBuffer, MIC_BUFFER_SIZE);
    
    if (success) {
      // Calculate RMS (Root Mean Square) for audio level
      int64_t sum = 0;
      for (size_t i = 0; i < MIC_BUFFER_SIZE; i++) {
        int32_t sample = micBuffer[i];
        sum += (int64_t)sample * sample;
      }
      
      int64_t mean = sum / MIC_BUFFER_SIZE;
      int rms = (int)sqrt(mean);
      
      // Normalize to 0-100 range
      // int16_t range is -32768 to 32767, adjust scaling
      currentAudioLevel = min(100, max(0, (rms * 100) / 10000));
    }
  }
  
  return currentAudioLevel;
}

bool isMicrophoneInitialized() {
  return micInitialized;
}

bool startRecording(const char* filename) {
  if (!micInitialized) {
    Serial.println("❌ ERROR: Microphone not initialized");
    return false;
  }
  
  if (recording) {
    Serial.println("⚠️  WARN: Already recording");
    return false;
  }
  
  // Create recordings directory if it doesn't exist
  if (!SD.exists("/recordings")) {
    SD.mkdir("/recordings");
  }
  
  String fullPath = String("/recordings/") + filename;
  recordingFile = SD.open(fullPath.c_str(), FILE_WRITE);
  
  if (!recordingFile) {
    Serial.printf("❌ ERROR: Failed to create recording file: %s\n", fullPath.c_str());
    return false;
  }
  
  // Write placeholder WAV header (we'll update it when done)
  WAVHeader header;
  header.fileSize = 0;
  header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
  header.dataSize = 0;
  
  recordingFile.write((uint8_t*)&header, sizeof(WAVHeader));
  
  recording = true;
  recordingStartTime = millis();
  recordingDataSize = 0;
  
  Serial.printf("🎙️  Recording started: %s\n", fullPath.c_str());
  return true;
}

void stopRecording() {
  if (!recording) {
    return;
  }
  
  recording = false;
  
  // Update WAV header with actual sizes
  WAVHeader header;
  header.fileSize = recordingDataSize + sizeof(WAVHeader) - 8;
  header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
  header.dataSize = recordingDataSize;
  
  recordingFile.seek(0);
  recordingFile.write((uint8_t*)&header, sizeof(WAVHeader));
  recordingFile.close();
  
  uint32_t duration = (millis() - recordingStartTime) / 1000;
  Serial.printf("🎙️  Recording stopped. Duration: %d seconds, Size: %d bytes\n", duration, recordingDataSize);
}

bool isRecording() {
  return recording;
}

int getRecordingDuration() {
  if (!recording) {
    return 0;
  }
  return (millis() - recordingStartTime) / 1000;
}

void processRecording() {
  if (!recording || !micInitialized) {
    return;
  }
  
  // Read audio data using M5Unified - directly writes to int16_t buffer
  bool success = M5.Mic.record(micBuffer, MIC_BUFFER_SIZE);
  
  if (success) {
    // Data is already in 16-bit format, write directly to file
    size_t bytesToWrite = MIC_BUFFER_SIZE * sizeof(int16_t);
    size_t written = recordingFile.write((uint8_t*)micBuffer, bytesToWrite);
    recordingDataSize += written;
    
    // Auto-stop if file gets too large (100MB limit)
    if (recordingDataSize > 100 * 1024 * 1024) {
      Serial.println("⚠️  WARN: Recording stopped - file size limit reached");
      stopRecording();
    }
  }
}

void setupButton() {
  Serial.println("ℹ️  INFO: Button initialized (using M5Unified API)");
}

bool readButton() {
  M5.update();
  return M5.BtnA.isPressed();
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ℹ️  INFO: RGB LED initialized on GPIO 35");
}

void setLED(int r, int g, int b) {
  rgbLedWrite(LED_PIN, r, g, b);
}

bool isReservedPin(int pin) {
  return (pin == LED_PIN || pin == MIC_DATA_PIN || pin == MIC_CLK_PIN || 
          pin == BUTTON_PIN || pin == SDCARD_MISO || pin == SDCARD_MOSI || 
          pin == SDCARD_SCK || pin == SDCARD_CS);
}

bool setGPIOMode(int pin, const String& mode) {
  if (isReservedPin(pin)) {
    return false;
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
    return false;
  }

  return true;
}

bool writeGPIO(int pin, int value) {
  if (isReservedPin(pin)) {
    return false;
  }

  digitalWrite(pin, value ? HIGH : LOW);
  return true;
}

int readGPIO(int pin) {
  return digitalRead(pin);
}

int readAnalogGPIO(int pin) {
  return analogRead(pin);
}

