#ifndef USB_HID_MODULE_H
#define USB_HID_MODULE_H

#include "module_base.h"

class USBHIDModule : public ModuleBase {
public:
    const char* getName() const override { return "usb_hid"; }
    const char* getVersion() const override { return "1.0.0"; }
    const char* getDescription() const override { return "USB HID keyboard/mouse/media control"; }
    
    bool init() override;
    void update() override;
    void registerAPI(AsyncWebServer& server) override;
    void getStatus(JsonObject& obj) const override;
    bool isReady() const override { return initialized && isActive; }
    
    bool enableHID();
    bool disableHID();
    
private:
    bool initialized = false;
    bool isActive = false;
};

#endif
