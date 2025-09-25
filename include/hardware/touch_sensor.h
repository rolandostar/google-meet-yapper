#ifndef TOUCH_SENSOR_H
#define TOUCH_SENSOR_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Event types that can be triggered by touch sensor
enum TouchEvent {
    TOUCH_PRESSED,
    TOUCH_RELEASED
};

// Callback function type
typedef void (*TouchCallback)(TouchEvent event);

class TouchSensor {
public:
    // Constructor
    TouchSensor(uint8_t touchPin, uint16_t debounceTime = DEBOUNCE_TIME);
    
    // Initialization
    void begin();
    
    // Calibration methods
    void startCalibration();
    bool isCalibrating() const { return calibrationInProgress; }
    bool isCalibrated() const { return calibrationComplete; }
    
    // Get calibration values
    int getUntouchedValue() const { return untouchedValue; }
    int getTouchedValue() const { return touchedValue; }
    
    // Register callback for touch events
    void setCallback(TouchCallback callback);
    
    // Update method to be called in the main loop
    void update();
    
    // Get current touch state
    bool isTouched() const { return touchState == 1; }
    
    // Get raw touch value
    int getRawValue() const;
    
    // Get threshold value
    int getThreshold() const { return touchThreshold; }

private:
    uint8_t touchPin;
    uint16_t debounceTime;
    int touchThreshold;
    int touchState;
    int lastReading;
    unsigned long lastDebounceTime;
    bool calibrationInProgress;
    bool calibrationComplete;
    unsigned long calibrationStartTime;
    int calibrationStage;  // 0=untouched, 1=touched
    int untouchedValue;
    int touchedValue;
    
    TouchCallback callback;
    Preferences preferences;
    
    void loadSettings();
    void saveSettings();
    void completeCalibration(int baselineValue);
};

// Global accessor function
TouchSensor& getTouchSensor();

#endif // TOUCH_SENSOR_H
