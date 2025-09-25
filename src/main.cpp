/*
 * ESP32 acting as a Multi-Client Bluetooth Telephony (Mute-Only) Controller
 * with LED Strip, Touch Sensor, Mechanical Buttons, and Rotary Encoder
 */

#include <Arduino.h>
#include "config.h"
#include "core/device_controller.h"

// Create the main device controller
DeviceController controller;

void setup() {
    // LOG_INIT();
    // LOG_INFO("ESP32 BLEHID Controller Starting...");
    // LOG_DEBUG("Compiled with log level: %d", LOG_LEVEL);
    
    controller.begin();
}

void loop() {
    controller.update();
    delay(10); // Small delay to prevent CPU overheating
}
