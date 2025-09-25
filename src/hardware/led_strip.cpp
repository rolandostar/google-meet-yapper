#include "hardware/led_strip.h"
#include "config.h"

// Singleton instance
LedStrip& getLedStrip() {
    // Default pins based on the example code
    static LedStrip instance(9, 11, 12); // 9 pixels, data pin 11, clock pin 12
    return instance;
}

LedStrip::LedStrip(uint16_t numPixels, uint8_t dataPin, uint8_t clockPin)
    : strip(numPixels, dataPin, clockPin, DOTSTAR_BRG), _brightness(0) {
}

void LedStrip::begin(uint8_t brightness) {
    strip.begin();
    
    // Load brightness from preferences first
    loadBrightness();
    
    // If no saved brightness or invalid brightness provided, use the parameter or default
    if (_brightness == 0) {
        _brightness = brightness;
        saveBrightness(); // Save the initial brightness
    }
    
    strip.setBrightness(_brightness);
    strip.show(); // Initialize all pixels to 'off'
}

void LedStrip::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    strip.setBrightness(_brightness);
    strip.show();
}

void LedStrip::setBrightnessAndSave(uint8_t brightness) {
    setBrightness(brightness);
    saveBrightness();
}

uint8_t LedStrip::getBrightness() const {
    return _brightness;
}

void LedStrip::setColor(uint32_t color) {
    strip.fill(color);
    strip.show();
}

void LedStrip::setColor(uint8_t r, uint8_t g, uint8_t b) {
    setColor(strip.Color(r, g, b));
}

void LedStrip::setPixelColor(uint16_t pixelIndex, uint32_t color) {
    strip.setPixelColor(pixelIndex, color);
}

void LedStrip::setPixelColor(uint16_t pixelIndex, uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(pixelIndex, r, g, b);
}

void LedStrip::clear() {
    strip.clear();
    strip.show();
}

void LedStrip::show() {
    strip.show();
}

void LedStrip::loadBrightness() {
    preferences.begin("led-settings", true); // Read-only mode
    _brightness = preferences.getUChar("brightness", LED_BRIGHTNESS); // Use config default if not found
    preferences.end();
}

void LedStrip::saveBrightness() {
    preferences.begin("led-settings", false); // Read-write mode
    preferences.putUChar("brightness", _brightness);
    preferences.end();
}


