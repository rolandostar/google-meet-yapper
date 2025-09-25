#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <Arduino.h>
#include <Adafruit_DotStar.h>
#include <Preferences.h>
#include <SPI.h>

class LedStrip {
public:
    // Constructor
    LedStrip(uint16_t numPixels, uint8_t dataPin, uint8_t clockPin);
    
    // Initialization
    void begin(uint8_t brightness = 80);
    
    // Control methods
    void setBrightness(uint8_t brightness);
    void setBrightnessAndSave(uint8_t brightness); // Set brightness and save to preferences
    uint8_t getBrightness() const;
    void setColor(uint32_t color);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint16_t pixelIndex, uint32_t color);
    void setPixelColor(uint16_t pixelIndex, uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void show();
    
    // Pre-defined colors
    uint32_t colorRed() { return strip.Color(255, 0, 0); }
    uint32_t colorGreen() { return strip.Color(0, 255, 0); }
    uint32_t colorBlue() { return strip.Color(0, 0, 255); }
    uint32_t colorYellow() { return strip.Color(255, 255, 0); }
    uint32_t colorMagenta() { return strip.Color(255, 0, 255); }
    uint32_t colorCyan() { return strip.Color(0, 255, 255); }
    uint32_t colorWhite() { return strip.Color(255, 255, 255); }
    uint32_t color(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }

private:
    Adafruit_DotStar strip;
    uint8_t _brightness;
    Preferences preferences;
    
    void loadBrightness();
    void saveBrightness();
};

// Global accessor function
LedStrip& getLedStrip();

#endif // LED_STRIP_H
