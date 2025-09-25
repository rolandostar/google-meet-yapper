#include "hardware/button.h"
#include "config.h"

Button::Button(uint8_t buttonPin, uint16_t debounceTime, uint16_t longPressTime, uint16_t doubleClickTime) 
    : pin(buttonPin), callback(nullptr) {
    
    // Setup custom configuration for PinButton
    // Note: In MultiButton, singleClickDelay is used for double-click detection window
    buttonConfig.debounceDecay = debounceTime;
    buttonConfig.longClickDelay = longPressTime;
    buttonConfig.singleClickDelay = doubleClickTime;
    
    // Create a new PinButton instance with the button pin and custom config
    button = new PinButton(pin, INPUT_PULLUP, &buttonConfig);
}

void Button::setCallback(ButtonCallback callback) {
    this->callback = callback;
}
void Button::update() {
    // Let PinButton handle the button processing
    button->update();
    
    // Check for button events and trigger callbacks
    // The MultiButton library only returns true for these methods
    // during the first call after the event occurs
    if (callback) {
      if (button->isSingleClick()) callback(BUTTON_CLICKED);
      if (button->isDoubleClick()) callback(BUTTON_DOUBLE_CLICKED);
      if (button->isLongClick()) callback(BUTTON_LONG_PRESSED);
      if (button->isReleased()) callback(BUTTON_RELEASED);
    }
}
