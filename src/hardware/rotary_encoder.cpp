#include "config.h"
#include "hardware/rotary_encoder.h"

//  Singleton instance
RotaryEncoder& getRotaryEncoder() {
    // Default pins for the ESP32-S3: GPIO5, GPIO6, GPIO7 for A, B, Button
    // Use a longer double click time (500ms instead of default 300ms)
    static RotaryEncoder instance(5, 6, 7, DEBOUNCE_TIME, LONG_PRESS_TIME, DOUBLE_CLICK_TIME);
    return instance;
}

RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t buttonPin, uint16_t debounceTime, uint16_t longPressTime, uint16_t doubleClickTime) 
    : pinA(pinA),
      pinB(pinB),
      buttonPin(buttonPin),
      lastCount(0),
      lastUpdateCount(0),
      clickButton(buttonPin, debounceTime, longPressTime, doubleClickTime),
      callback(nullptr) {
}

void RotaryEncoder::begin() {
    // Configure encoder pins
    ESP32Encoder::useInternalWeakPullResistors = UP;
    encoder.attachSingleEdge(pinA, pinB);
    encoder.clearCount();
}

void RotaryEncoder::setCallback(EncoderCallback callback) {
    this->callback = callback;
}

int64_t RotaryEncoder::getCount() {
    return encoder.getCount();
}

void RotaryEncoder::update() {
    // Handle encoder rotation
    int64_t currentCount = encoder.getCount();
    
    if (currentCount != lastUpdateCount) {
        EncoderEvent event = (currentCount > lastUpdateCount) ? ENCODER_CLOCKWISE : ENCODER_COUNTER_CLOCKWISE;
        handleEncoderEvent(event);
        lastUpdateCount = currentCount;
    }
    
    // Handle precision mode timeout (reset accumulator if no activity)
    if (notchAccumulator > 0) {
        if (millis() - lastNotchTime >= ENCODER_PRECISION_RESET_TIMEOUT) {
            notchAccumulator = 0;
        }
    }
    
    // Update the button state through the mechanical button instance
    clickButton.update();
}

void RotaryEncoder::handleEncoderEvent(EncoderEvent event) {
    // Apply directional bounce filtering
    if (event == lastDirection) {
        consistentDirectionCount++;
    } else {
        // Direction changed - require consistency before accepting
        consistentDirectionCount = 1;
        lastDirection = event;
    }
    
    // Only process the event if we have consistent direction readings
    if (consistentDirectionCount < ENCODER_DIRECTION_CONSISTENCY) {
        return; // Ignore until we get consistent readings
    }
    
    // Precision mode: accumulate notches for one arrow
    unsigned long currentTime = millis();
    
    // Reset accumulator if direction changed or timeout occurred
    if (event != accumulatedDirection || 
        (currentTime - lastNotchTime >= ENCODER_PRECISION_RESET_TIMEOUT)) {
        notchAccumulator = 0;
        accumulatedDirection = event;
    }
    
    // Increment notch counter
    notchAccumulator++;
    lastNotchTime = currentTime;
    
    // Send callback when threshold is reached
    if (notchAccumulator >= ENCODER_PRECISION_NOTCH_THRESHOLD) {
        if (callback) {
            callback(event);
        }
        notchAccumulator = 0; // Reset for next accumulation
    }
}

Button& RotaryEncoder::getClickButton() {
    return clickButton;
}
