#include "communication/keyboard_handler.h"
#include "communication/bluetooth_handler.h"
#include "hidmap.h"
#include "config.h"

#define HID_REPORTID_KEYBOARD_INPUT  0x03

// Static accessor function
KeyboardHandler& getKeyboardHandler() {
    return KeyboardHandler::getInstance();
}

KeyboardHandler::KeyboardHandler() {
}

void KeyboardHandler::sendRightArrow() {
    // Right arrow key - no modifiers
    sendKeys(0, KEY_RIGHT_ARROW);
    delay(200);  // Small delay for key press
    releaseAllKeys();
}

void KeyboardHandler::sendLeftArrow() {
    // Left arrow key - no modifiers
    sendKeys(0, KEY_LEFT_ARROW);
    delay(200);  // Small delay for key press
    releaseAllKeys();
}

void KeyboardHandler::sendCtrlShiftF1() {
    // CTRL+SHIFT+F1 - combining CTRL and SHIFT with F1
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_SHIFT, KEY_F1);
    delay(200);  // Small delay for key press
    releaseAllKeys();
}

void KeyboardHandler::sendCtrlE() {
    // CTRL+E - combining CTRL with E
    sendKeys(KEY_LEFT_CTRL, KEY_E);
    delay(200);  // Small delay for key press
    releaseAllKeys();
}

void KeyboardHandler::sendCtrlAltH() {
    // CTRL+ALT+H - combining CTRL and ALT with H
    // Try shorter delay first - Google Meet might be timing-sensitive
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT);
    delay(50);
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT, KEY_H);
    delay(50);
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT);
    delay(50);
    releaseAllKeys();
}

void KeyboardHandler::sendCtrlAltHAlternative1() {
    // Alternative 1: Sequential key presses
    LOG_DEBUG("Sending Ctrl+Alt+H (Sequential method)");
    
    // Press and hold modifiers first
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT, 0);
    delay(50);
    
    // Add H key while holding modifiers
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT, KEY_H);
    delay(100);
    
    // Release all keys
    releaseAllKeys();
    delay(50);
}

void KeyboardHandler::sendCtrlAltHAlternative2() {
    // Alternative 2: Very short timing (more like a real keypress)
    LOG_DEBUG("Sending Ctrl+Alt+H (Fast timing method)");
    
    sendKeys(KEY_LEFT_CTRL | KEY_LEFT_ALT, KEY_H);
    delay(50);   // Very short delay
    releaseAllKeys();
    delay(25);
}

void KeyboardHandler::sendCtrlAltHAlternative3() {
    // Alternative 3: Use right modifiers instead of left
    LOG_DEBUG("Sending Ctrl+Alt+H (Right modifiers method)");
    
    sendKeys(KEY_RIGHT_CTRL | KEY_RIGHT_ALT, KEY_H);
    delay(100);
    releaseAllKeys();
    delay(50);
}

void KeyboardHandler::sendA() {
    // Send 'a' key - no modifiers
    sendKeys(0, KEY_A);
    delay(200);  // Small delay for key press
    releaseAllKeys();
}

void KeyboardHandler::sendShortcut(uint8_t shortcutType) {
    switch (shortcutType) {
        case SHORTCUT_RIGHT_ARROW:
            sendRightArrow();
            break;
        case SHORTCUT_LEFT_ARROW:
            sendLeftArrow();
            break;
        case SHORTCUT_CTRL_SHIFT_F1:
            sendCtrlShiftF1();
            break;
        case SHORTCUT_CTRL_E:
            sendCtrlE();
            break;
        case SHORTCUT_CTRL_ALT_H:
            sendCtrlAltH();
            break;
        case SHORTCUT_CTRL_ALT_H_ALT1:
            sendCtrlAltHAlternative1();
            break;
        case SHORTCUT_CTRL_ALT_H_ALT2:
            sendCtrlAltHAlternative2();
            break;
        case SHORTCUT_CTRL_ALT_H_ALT3:
            sendCtrlAltHAlternative3();
            break;
        default:
            LOG_WARN("Unknown shortcut type: %d", shortcutType);
            break;
    }
}

bool KeyboardHandler::sendKeys(uint8_t modifiers, uint8_t key1, uint8_t key2, uint8_t key3, uint8_t key4, uint8_t key5) {
    // Standard keyboard report structure: [modifier, reserved, key1, key2, key3, key4, key5, key6]
    uint8_t keyReport[8] = { modifiers, 0, key1, key2, key3, key4, key5, 0 };
    
    // Debug output
    LOG_DEBUG("Sending keyboard report: [%02X %02X %02X %02X %02X %02X %02X %02X]", 
                  keyReport[0], keyReport[1], keyReport[2], keyReport[3],
                  keyReport[4], keyReport[5], keyReport[6], keyReport[7]);
    
    return getBLEHandler().sendKeyboardReport(keyReport);
}

void KeyboardHandler::releaseAllKeys() {
    // Send empty report to release all keys
    uint8_t keyReport[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    getBLEHandler().sendKeyboardReport(keyReport);
}

// --- Consumer Control Methods ---
void KeyboardHandler::sendVolumeUp() {
    LOG_DEBUG("Sending Volume Up");
    // Send volume up command (bit 0 set)
    getBLEHandler().sendConsumerReport(CONSUMER_VOLUME_UP);
    delay(50);  // Brief press
    // Send release (all bits clear)
    getBLEHandler().sendConsumerReport(0x00);
}

void KeyboardHandler::sendVolumeDown() {
    LOG_DEBUG("Sending Volume Down");
    // Send volume down command (bit 1 set)
    getBLEHandler().sendConsumerReport(CONSUMER_VOLUME_DOWN);
    delay(50);  // Brief press
    // Send release (all bits clear)
    getBLEHandler().sendConsumerReport(0x00);
}

void KeyboardHandler::sendConsumerMute() {
    LOG_DEBUG("Sending Consumer Mute");
    // Send mute command (bit 2 set)
    getBLEHandler().sendConsumerReport(CONSUMER_MUTE);
    delay(50);  // Brief press
    // Send release (all bits clear)
    getBLEHandler().sendConsumerReport(0x00);
}