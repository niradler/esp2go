#ifndef BUTTON_MODULE_H
#define BUTTON_MODULE_H

#include "module_base.h"

class ButtonModule : public ModuleBase {
public:
    const char* getName() const override { return "button"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "Button input monitoring"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    void getStatus(JsonObject& obj) const override;
    bool isReady() const override { return true; }
    
    bool isPressed();
    
private:
    bool buttonState = false;
    bool lastButtonState = false;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 50;
};

#endif

