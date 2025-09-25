#ifndef CONFIG_H
// Rotary Encoder Precision Mode Settings
#define ENCODER_DIRECTION_CONSISTENCY 2    // number of consistent direction readings needed to accept direction change
#define ENCODER_PRECISION_NOTCH_THRESHOLD 2 // number of notches needed to trigger one arrow in precision mode
#define ENCODER_PRECISION_RESET_TIMEOUT 500 // milliseconds - time after which notch accumulator resets

#include "logger.h"

// --- Custom Device Information ---
#define DEVICE_MANUFACTURER "Custom Gadgets Inc."
#define DEVICE_NAME         "ESP32 Mute Control"
#define DEVICE_VID          0xEDFE // Vendor ID for hobbyist projects
#define DEVICE_PID          0xDEC0 // Product ID
#define DEVICE_VERSION      0x0100 // Version 1.00

// LED Settings
#define LED_BRIGHTNESS 10        // 0-255

// Input Settings
#define DEBOUNCE_TIME 50         // milliseconds
#define LONG_PRESS_TIME 700      // milliseconds
#define DOUBLE_CLICK_TIME 300    // milliseconds
#define LEFT_BUTTON_PIN   13      // Press to toggle mute
#define RIGHT_BUTTON_PIN   14     // Press to toggle drop call state (formerly hook button)

// Rotary Encoder Precision Mode Settings
#define ENCODER_NON_PRECISION_TIMEOUT 150  // milliseconds - time to wait before sending action in non-precision mode
#define ENCODER_DIRECTION_CONSISTENCY 2    // number of consistent direction readings needed to accept direction change
#define ENCODER_PRECISION_NOTCH_THRESHOLD 2 // number of notches (1-3) needed to trigger one arrow in precision mode
#define ENCODER_PRECISION_RESET_TIMEOUT 500 // milliseconds - time after which notch accumulator resets

// BLE Settings
#define MAX_BLE_CONNECTIONS 3    // Maximum simultaneous BLE connections
#define HID_HEADSET 0x0941       // Standard BLE appearance for a headset

// Animation Settings
#define LED_ANIMATION_SPEED 100  // milliseconds

// Touch Sensor Settings
#define CALIBRATION_INTERVAL 5000 // milliseconds

#endif // CONFIG_H
