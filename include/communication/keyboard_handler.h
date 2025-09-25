#ifndef KEYBOARD_HANDLER_H
#define KEYBOARD_HANDLER_H

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEHIDDevice.h"

// HID Keyboard Modifier Key Bit Positions (for first byte of report)
#define KEY_NONE           0x00
#define KEY_LEFT_CTRL      0x01  // Bit 0
#define KEY_LEFT_SHIFT     0x02  // Bit 1
#define KEY_LEFT_ALT       0x04  // Bit 2
#define KEY_LEFT_GUI       0x08  // Bit 3
#define KEY_RIGHT_CTRL     0x10  // Bit 4
#define KEY_RIGHT_SHIFT    0x20  // Bit 5
#define KEY_RIGHT_ALT      0x40  // Bit 6
#define KEY_RIGHT_GUI      0x80  // Bit 7

// Common keys (HID Usage IDs for keyboard/keypad page)
#define KEY_A           0x04
#define KEY_E           0x08
#define KEY_H           0x0B
#define KEY_LEFT_ARROW  0x50
#define KEY_RIGHT_ARROW 0x4F
#define KEY_UP_ARROW    0x52
#define KEY_DOWN_ARROW  0x51
#define KEY_F1          0x3A

// Shortcut definitions
#define SHORTCUT_RIGHT_ARROW      1
#define SHORTCUT_LEFT_ARROW       2
#define SHORTCUT_CTRL_SHIFT_F1    3
#define SHORTCUT_CTRL_E           4
#define SHORTCUT_CTRL_ALT_H       5
#define SHORTCUT_CTRL_ALT_H_ALT1  6  // Sequential method
#define SHORTCUT_CTRL_ALT_H_ALT2  7  // Fast timing method  
#define SHORTCUT_CTRL_ALT_H_ALT3  8  // Right modifiers method

class KeyboardHandler {
public:
    KeyboardHandler();
    
    // Methods for sending key combinations
    void sendRightArrow();
    void sendLeftArrow();
    void sendCtrlShiftF1();
    void sendCtrlE();
    void sendCtrlAltH();
    void sendCtrlAltHAlternative1();  // Sequential key presses
    void sendCtrlAltHAlternative2();  // Different timing
    void sendCtrlAltHAlternative3();  // Right modifiers instead of left
    void sendA();
    
    // Consumer control methods
    void sendVolumeUp();
    void sendVolumeDown();
    void sendConsumerMute();
    
    // Generic method for sending key shortcuts
    void sendShortcut(uint8_t shortcutType);
    
    // Method to send raw key presses
    bool sendKeys(uint8_t modifiers = 0, uint8_t key1 = 0, uint8_t key2 = 0, uint8_t key3 = 0, uint8_t key4 = 0, uint8_t key5 = 0);
    
    // Release all keys
    void releaseAllKeys();
    
    // Singleton instance getter
    static KeyboardHandler& getInstance() {
        static KeyboardHandler instance;
        return instance;
    }
    
private:
    // Send key report
    bool sendReport(uint8_t* report, size_t length);
};

// Static accessor function
KeyboardHandler& getKeyboardHandler();

#endif // KEYBOARD_HANDLER_H
