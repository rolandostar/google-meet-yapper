#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <Arduino.h>
#include "hardware/touch_sensor.h"
#include "hardware/led_strip.h"

class SerialHandler {
public:
    // Initialize the serial handler
    void begin(unsigned long baudRate = 115200);
    
    // Process any incoming serial commands
    void update();
    
    // Print help message with available commands
    void printHelpMessage();
    
    // Print touch sensor status
    void printTouchSensorStatus();
    
    // Print LED strip status
    void printLedStatus();

private:
    String commandBuffer;  // Buffer to store incoming command string
    
    // Process a specific command character
    void processCommand(char command);
    
    // Process a complete command string
    void processCommandString(const String& command);
};

// Singleton instance access
SerialHandler& getSerialHandler();

#endif // SERIAL_HANDLER_H
