#ifndef LED_MODULE_H
#define LED_MODULE_H

#include "module_base.h"

class LEDModule : public ModuleBase {
public:
    const char* getName() const override { return "led"; }
    const char* getDescription() const override { return "RGB LED Control"; }
    
    bool init() override;
    void registerAPI(AsyncWebServer& server) override;
    void update() override;
    bool isReady() const override { return initialized; }
    void getStatus(JsonObject& obj) const override;
    
    // LED Functions
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void off();
    void startBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t interval, int count = 0);
    void stopBlink();
    
private:
    bool initialized = false;
    uint8_t currentR = 0, currentG = 0, currentB = 0;
    uint8_t brightness = 255;
    bool isOn = false;
    
    // Blink state
    bool blinkActive = false;
    bool blinkState = false;
    uint32_t lastBlinkTime = 0;
    uint32_t blinkInterval = 500;
    int blinkCount = 0;
    int blinkCurrent = 0;
};

#endif

