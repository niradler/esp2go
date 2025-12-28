#ifndef MICROPHONE_MODULE_H
#define MICROPHONE_MODULE_H

#include "module_base.h"

class MicrophoneModule : public ModuleBase {
public:
    const char* getName() const override { return "microphone"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "Microphone audio input"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    void getStatus(JsonObject& obj) const override;
    bool isReady() const override { return true; }
    
    int getLevel();
    bool startRecording();
    bool stopRecording();
    bool getRecordingStatus();
    
private:
    int currentLevel = 0;
    bool isRecording = false;
    uint32_t recordingStartTime = 0;
    uint32_t maxRecordingDuration = 300000; // 5 minutes max
};

#endif

