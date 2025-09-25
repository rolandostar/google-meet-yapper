#include "hardware/touch_sensor.h"
#include "hardware/led_strip.h"
#include "config.h"

// Singleton instance
TouchSensor& getTouchSensor() {
    static TouchSensor instance(4);  // Default touch pin is 4
    return instance;
}

TouchSensor::TouchSensor(uint8_t touchPin, uint16_t debounceTime) 
    : touchPin(touchPin),
      debounceTime(debounceTime),
      touchThreshold(0),
      touchState(0),
      lastReading(0),
      lastDebounceTime(0),
      calibrationInProgress(false),
      calibrationComplete(false),
      calibrationStartTime(0),
      calibrationStage(0),
      untouchedValue(0),
      touchedValue(0),
      callback(nullptr) {
}

void TouchSensor::begin() {
    loadSettings();
    
    // If not calibrated, automatically start calibration
    if (!calibrationComplete) {
        startCalibration();
    }
}

void TouchSensor::loadSettings() {
    // Open preferences in read-only mode
    preferences.begin("touch-settings", true);
    
    // Load saved values or use defaults
    untouchedValue = preferences.getUInt("untouched", 0);
    touchedValue = preferences.getUInt("touched", 0);
    touchThreshold = preferences.getUInt("touchThresh", 0);
    
    // If we have both untouched and touched values, we can calculate a threshold
    if (untouchedValue > 0 && touchedValue > 0) {
        calibrationComplete = true;
    } else {
        calibrationComplete = false;
        touchThreshold = 0;
    }
    
    preferences.end();
}

void TouchSensor::saveSettings() {
    preferences.begin("touch-settings", false);
    
    preferences.putUInt("untouched", untouchedValue);
    preferences.putUInt("touched", touchedValue);
    preferences.putUInt("touchThresh", touchThreshold);
    
    preferences.end();
    
    LOG_DEBUG("Touch sensor settings saved");
}

void TouchSensor::startCalibration() {
    LOG_INFO("--- Starting Touch Calibration ---");
    LOG_INFO("Calibrating UNTOUCHED state...");
    LOG_INFO(">>> DO NOT TOUCH the sensor for 5 seconds. <<<");
    
    // Set LED to blue during calibration
    getLedStrip().setColor(getLedStrip().colorBlue());
    
    calibrationInProgress = true;
    calibrationStage = 0; // Start with untouched calibration
    calibrationStartTime = millis();
    calibrationComplete = false;
}

void TouchSensor::completeCalibration(int baselineValue) {
    if (calibrationStage == 0) {
        // Store the untouched baseline value
        untouchedValue = baselineValue;
        
        // Move to next stage - touched calibration
        calibrationStage = 1;
        calibrationStartTime = millis();
        
        // Set LED to magenta (purple-ish) to indicate touched calibration phase
        getLedStrip().setColor(getLedStrip().colorMagenta());
        
        LOG_INFO("--- Now Calibrating TOUCHED state ---");
        LOG_INFO(">>> TOUCH and HOLD the sensor for 5 seconds. <<<");
        
        return; // Don't complete calibration yet
    } else {
        // Store the touched value
        touchedValue = baselineValue;
        
        // Calculate the threshold as the midpoint between untouched and touched values
        if (touchedValue > untouchedValue) {
            // Normal case: touched value is higher
            touchThreshold = untouchedValue + ((touchedValue - untouchedValue) / 2);
        } else {
            // Inverted case: untouched value is higher (unusual but possible)
            touchThreshold = touchedValue + ((untouchedValue - touchedValue) / 2);
        }
        
        // Save all settings
        calibrationComplete = true;
        saveSettings();
        
        LOG_INFO("-------------------------------------------------");
        LOG_INFO("Calibration Complete and Saved!");
        LOG_INFO("Untouched baseline: %u", untouchedValue);
        LOG_INFO("Touched value: %u", touchedValue);
        LOG_INFO("New Touch Threshold set to: %u", touchThreshold);
        LOG_INFO("-------------------------------------------------");
        
        calibrationInProgress = false;
        
        // Reset LED after calibration
        getLedStrip().clear();
    }
}

int TouchSensor::getRawValue() const {
    return touchRead(touchPin);
}

void TouchSensor::setCallback(TouchCallback callback) {
    this->callback = callback;
}

void TouchSensor::update() {
    unsigned long currentMillis = millis();
    
    // Handle calibration if it's in progress
    if (calibrationInProgress) {
        // Give user time (5 seconds total) for the current calibration stage
        if ((currentMillis - calibrationStartTime) > CALIBRATION_INTERVAL) {
            // Take samples to establish baseline
            long sampleSum = 0;
            int calibrationSamples = 200;
            for (int i = 0; i < calibrationSamples; i++) {
                sampleSum += touchRead(touchPin);
                delay(5);
            }
            int sampleAverage = sampleSum / calibrationSamples;
            
            if (calibrationStage == 0) {
                LOG_DEBUG("Untouched Average (Baseline): %d", sampleAverage);
            } else {
                LOG_DEBUG("Touched Average: %d", sampleAverage);
            }
            
            completeCalibration(sampleAverage);
            return;
        }
        
        // Different blink rates for different calibration stages
        int blinkRate = (calibrationStage == 0) ? 500 : 250;
        
        // During calibration, blink LED
        if ((currentMillis / blinkRate) % 2) {
            if (calibrationStage == 0) {
                getLedStrip().setColor(getLedStrip().colorBlue());
            } else {
                getLedStrip().setColor(getLedStrip().colorMagenta());
            }
        } else {
            getLedStrip().clear();
        }
        
        return;  // Skip the rest of the update during calibration
    }
    
    // Skip touch detection if not calibrated
    if (!calibrationComplete) {
        return;
    }
    
    // Read touch value and determine current state
    int touchValue = touchRead(touchPin);
    // Determine if touched based on threshold
    int currentReading = (touchValue > touchThreshold) ? 1 : 0;
    
    // Debounce
    if (currentReading != lastReading) {
        lastDebounceTime = currentMillis;
    }
    
    if ((currentMillis - lastDebounceTime) > debounceTime) {
        if (currentReading != touchState) {
            touchState = currentReading;
            
            // Notify of touch events
            if (callback) {
                if (touchState) {
                    callback(TOUCH_PRESSED);
                } else {
                    callback(TOUCH_RELEASED);
                }
            }
        }
    }
    
    lastReading = currentReading;
    
    // For debugging
    /*
    Serial.print("Touch:");
    Serial.print(touchValue);
    Serial.print(", Thresh:");
    Serial.print(touchThreshold);
    Serial.print(", State:");
    Serial.println(touchState);
    */
}
