#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <PinButton.h>
#include "config.h"

// Event types that can be triggered by buttons
enum ButtonEvent {
    BUTTON_PRESSED,
    BUTTON_RELEASED,
    BUTTON_CLICKED,
    BUTTON_DOUBLE_CLICKED,
    BUTTON_LONG_PRESSED
};

// Callback function type
typedef void (*ButtonCallback)(ButtonEvent event);

class Button {
public:
    // Constructor
    Button(uint8_t buttonPin, uint16_t debounceTime = DEBOUNCE_TIME, uint16_t longPressTime = LONG_PRESS_TIME, uint16_t doubleClickTime = DOUBLE_CLICK_TIME);
    
    // Initialization
    void begin();
    
    // Register callback for button events
    void setCallback(ButtonCallback callback);
    
    // Update method to be called in the main loop
    void update();

private:
    uint8_t pin;
    PinButton* button;
    ButtonCallback callback;
    
    // Custom configuration for the button
    MultiButtonConfig buttonConfig;
};

#endif // BUTTON_H
