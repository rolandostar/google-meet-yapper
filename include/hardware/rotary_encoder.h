#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>
#include <ESP32Encoder.h>
#include "button.h"

// Event types that can be triggered by the rotary encoder
enum EncoderEvent {
    ENCODER_CLOCKWISE,
    ENCODER_COUNTER_CLOCKWISE
};

// Callback function type
typedef void (*EncoderCallback)(EncoderEvent event);

class RotaryEncoder {
public:
    // Constructor
    RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t buttonPin, uint16_t debounceTime = DEBOUNCE_TIME, uint16_t longPressTime = LONG_PRESS_TIME, uint16_t doubleClickTime = DOUBLE_CLICK_TIME);
    
    // Initialization
    void begin();
    
    // Register callback for encoder events
    void setCallback(EncoderCallback callback);
    
    // Update method to be called in the main loop
    void update();
    
    // Get current encoder values
    int64_t getCount();
    
    // Get access to the click button instance
    Button& getClickButton();

private:
    uint8_t pinA, pinB, buttonPin;
    ESP32Encoder encoder;
    int64_t lastCount;
    int64_t lastUpdateCount;
    
    // Button instance for the rotary encoder's click
    Button clickButton;
    
    EncoderCallback callback;
    
    // Precision mode variables for accumulation
    int notchAccumulator = 0;
    EncoderEvent accumulatedDirection = ENCODER_CLOCKWISE;
    unsigned long lastNotchTime = 0;
    
    // Directional bounce filtering
    EncoderEvent lastDirection = ENCODER_CLOCKWISE;
    int consistentDirectionCount = 0;
    
    void handleButtonState(int reading);
    void handleEncoderEvent(EncoderEvent event);
};

// Global accessor function
RotaryEncoder& getRotaryEncoder();

#endif // ROTARY_ENCODER_H
