#include "communication/serial_handler.h"
#include "hardware/touch_sensor.h"
#include "hardware/led_strip.h"
#include "config.h"

// Singleton instance
SerialHandler& getSerialHandler() {
    static SerialHandler instance;
    return instance;
}

void SerialHandler::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    // Print available serial commands
    printHelpMessage();
}

void SerialHandler::update() {
    // Check if there are any commands from the Serial port
    while (Serial.available() > 0) {
        char incomingChar = Serial.read();
        
        if (incomingChar == '\n' || incomingChar == '\r') {
            // Process complete command
            if (commandBuffer.length() > 0) {
                processCommandString(commandBuffer);
                commandBuffer = ""; // Clear buffer
            }
        } else if (incomingChar >= 32 && incomingChar <= 126) { // Printable characters
            commandBuffer += incomingChar;
        }
        
        // Fallback for single character commands
        if (commandBuffer.length() == 1 && (incomingChar == 'c' || incomingChar == 'h')) {
            processCommand(incomingChar);
            commandBuffer = "";
        }
    }
}

void SerialHandler::processCommand(char command) {
    // Process single character commands
    switch (command) {
        case 'c':
            LOG_INFO("Serial command 'c' received: Starting touch sensor calibration");
            getTouchSensor().startCalibration();
            break;
            
        case 'h':
            // Print help message and sensor status
            printHelpMessage();
            printTouchSensorStatus();
            printLedStatus();
            break;
            
        default:
            LOG_WARN("Unknown command. Type 'h' for help.");
            break;
    }
}

void SerialHandler::processCommandString(const String& command) {
    // Process string commands like "b255"
    if (command.length() == 0) return;
    
    char firstChar = command.charAt(0);
    
    switch (firstChar) {
        case 'b': {
            // Brightness command: b0 to b255
            if (command.length() > 1) {
                int brightness = command.substring(1).toInt();
                if (brightness >= 0 && brightness <= 255) {
                    getLedStrip().setBrightnessAndSave(brightness);
                    Serial.print("LED brightness set to: ");
                    Serial.println(brightness);
                    LOG_INFO("LED brightness changed to: %d", brightness);
                } else {
                    Serial.println("Error: Brightness must be 0-255");
                }
            } else {
                Serial.print("Current LED brightness: ");
                Serial.println(getLedStrip().getBrightness());
            }
            break;
        }
        
        case 'c':
            LOG_INFO("Serial command 'c' received: Starting touch sensor calibration");
            getTouchSensor().startCalibration();
            break;
            
        case 'h':
            printHelpMessage();
            printTouchSensorStatus();
            printLedStatus();
            break;
            
        default:
            Serial.print("Unknown command: ");
            Serial.println(command);
            Serial.println("Type 'h' for help.");
            break;
    }
}

void SerialHandler::printHelpMessage() {
  Serial.println("------ Available Serial Commands ------");
  Serial.println("c - Start touch sensor calibration");
  Serial.println("h - Display this help message");
  Serial.println("b[0-255] - Set LED brightness (e.g., b255, b128, b0)");
  Serial.println("b - Show current LED brightness");
  Serial.println("------------------------------------");
}

void SerialHandler::printTouchSensorStatus() {
  Serial.println("------ Touch Sensor Status ------");
  Serial.print("Calibrated: ");
  Serial.println(getTouchSensor().isCalibrated() ? "YES" : "NO");
  
  if (getTouchSensor().isCalibrated()) {
    Serial.print("Untouched value: ");
    Serial.println(getTouchSensor().getUntouchedValue());
    Serial.print("Touched value: ");
    Serial.println(getTouchSensor().getTouchedValue());
    Serial.print("Threshold: ");
    Serial.println(getTouchSensor().getThreshold());
    Serial.print("Current raw value: ");
    Serial.println(getTouchSensor().getRawValue());
  }
  
  Serial.println("-------------------------------");
}

void SerialHandler::printLedStatus() {
  Serial.println("------ LED Strip Status ------");
  Serial.print("Current brightness: ");
  Serial.print(getLedStrip().getBrightness());
  Serial.println("/255");
  Serial.println("-------------------------------");
}
