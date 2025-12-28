#ifndef GPIO_MODULE_H
#define GPIO_MODULE_H

#include "module_base.h"
#include <map>

class GPIOModule : public ModuleBase {
public:
    const char* getName() const override { return "gpio"; }
    const char* getDescription() const override { return "General Purpose I/O Control"; }
    
    bool init() override;
    void registerAPI(AsyncWebServer& server) override;
    bool isReady() const override { return initialized; }
    void getStatus(JsonObject& obj) const override;
    
    // GPIO Functions
    bool pinMode(uint8_t pin, uint8_t mode);
    bool digitalWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);
    int analogRead(uint8_t pin);
    bool analogWrite(uint8_t pin, uint16_t value);  // PWM
    
    // Pin availability check
    bool isPinAvailable(uint8_t pin);
    bool isPinADC(uint8_t pin);
    bool isPinPWM(uint8_t pin);
    
    // Get available pins
    void getAvailablePins(JsonArray& array);
    void getReservedPins(JsonArray& array);
    
private:
    bool initialized = false;
    std::map<uint8_t, uint8_t> pinModes;  // Track pin modes
    
    // Reserved pins (SD card, LED, Button, etc.)
    const uint8_t reservedPins[10] = {35, 36, 37, 38, 39, 40, 41};
    
    // Available GPIO pins on AtomS3
    const uint8_t availablePins[12] = {1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    
    // ADC capable pins
    const uint8_t adcPins[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // PWM capable pins (most ESP32 pins)
    const uint8_t pwmPins[12] = {1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
};

#endif

