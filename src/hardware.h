#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

// Hardware initialization
void setupMicrophone();
void setupButton();
void setupLED();

// Hardware reading
int readMicrophoneLevel();
bool readButton();
bool isMicrophoneInitialized();

// Audio recording
bool startRecording(const char* filename);
void stopRecording();
bool isRecording();
int getRecordingDuration(); // in seconds
void processRecording(); // Call this in loop() to write audio data

// Hardware control
void setLED(int r, int g, int b);

// GPIO control
bool setGPIOMode(int pin, const String& mode);
bool writeGPIO(int pin, int value);
int readGPIO(int pin);
int readAnalogGPIO(int pin);
bool isReservedPin(int pin);

#endif

