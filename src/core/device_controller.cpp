#include "core/device_controller.h"
#include "communication/bluetooth_handler.h"
#include "communication/serial_handler.h"
#include "communication/keyboard_handler.h"
#include "hardware/led_strip.h"
#include "hardware/touch_sensor.h"
#include "hardware/rotary_encoder.h"
#include "config.h"

// Initialize static instance pointer
DeviceController* DeviceController::instance = nullptr;

DeviceController::DeviceController() 
    : leftButton(LEFT_BUTTON_PIN)
    , rightButton(RIGHT_BUTTON_PIN) {
    
    // Set the static instance pointer
    instance = this;
    
    // Set up callbacks using static member functions
    leftButton.setCallback(staticLeftButtonCallback);
    rightButton.setCallback(staticRightButtonCallback);
}

void DeviceController::begin() {
    // Initialize all components
    getSerialHandler().begin(115200);
    getLedStrip().begin(LED_BRIGHTNESS);
    getTouchSensor().begin();
    getTouchSensor().setCallback(staticTouchCallback);
    getRotaryEncoder().begin();
    getRotaryEncoder().setCallback(staticEncoderCallback);
    getRotaryEncoder().getClickButton().setCallback(staticEncoderButtonCallback);
    getBLEHandler().begin();
    
    // Register callback for host state updates from Bluetooth
    getBLEHandler().setHostStateCallback(staticHostStateCallback);
}

void DeviceController::update() {
    leftButton.update();
    rightButton.update();
    getTouchSensor().update();
    getRotaryEncoder().update();
    getSerialHandler().update();
}

void DeviceController::updateCallState(bool muteValue, bool dropValue) {
    uint8_t reportValue = (muteValue ? 0x01 : 0x00) | (dropValue ? 0x02 : 0x00);
    
    if (getBLEHandler().sendHeadsetReport(reportValue)) {
        LOG_DEBUG("Call %s: %s", 
              callActive ? "Active" : "Idle",
              muteValue ? "Muted" : "Unmuted");
    }
}

// --- Left Button Event Handler ---
void DeviceController::onLeftButtonEvent(ButtonEvent event) {
    if (!callActive) {
        LOG_DEBUG("Left button ignored - no active call");
        return;
    }
    if (event == BUTTON_CLICKED) {
        // Send Ctrl+Shift+F1 keyboard command
        LOG_INFO("Left button clicked: Sending Ctrl+Shift+F1");
        getKeyboardHandler().sendCtrlShiftF1();
    } else if (event == BUTTON_LONG_PRESSED) {
        // Send hang up/drop call command
        LOG_INFO("Left button long pressed: Sending hang up/drop call command");
        updateCallState(muteState, true);
        delay(100);    // Small delay to ensure the signal is registered
        updateCallState(muteState, false);
    }
}

// --- Right Button Event Handler ---
void DeviceController::onRightButtonEvent(ButtonEvent event) {
    if (event == BUTTON_CLICKED) {
        // Only allow drop call if there's an active call
        if (!callActive) {
            LOG_DEBUG("Right button ignored - no active call");
            return;
        }
        
        LOG_INFO("Right button clicked: Sending Ctrl+Alt+H");
        getKeyboardHandler().sendCtrlAltH();
    }
}

// --- Encoder Button Event Handler ---
void DeviceController::onEncoderButtonEvent(ButtonEvent event) {
    LOG_DEBUG("Encoder button event: %d", event);
    
    if (event == BUTTON_CLICKED) {
        // Single click: toggle between volume control and arrow keys (only when not in call)
        if (!callActive) {
            toggleEncoderMode();
            LOG_INFO("Encoder clicked - Switched to %s mode", 
                     encoderVolumeMode ? "Volume Control" : "Arrow Keys");
            
            // Flash LED to indicate mode change
            if (encoderVolumeMode) {
                // Green flash for Volume Control mode
                getLedStrip().setColor(getLedStrip().colorGreen());
                delay(150);
                getLedStrip().clear();
                delay(150);
                getLedStrip().setColor(getLedStrip().colorGreen());
                delay(150);
                getLedStrip().clear();
            } else {
                // Orange flash for Arrow Keys mode
                getLedStrip().setColor(255, 165, 0); // Orange
                delay(150);
                getLedStrip().clear();
                delay(150);
                getLedStrip().setColor(255, 165, 0);
                delay(150);
                getLedStrip().clear();
            }
        } else {
            LOG_DEBUG("Encoder click ignored - call is active");
        }
    }
    else if (event == BUTTON_DOUBLE_CLICKED) {
        // Toggle push-to-talk mode
        togglePushToTalk();
        LOG_DEBUG("Encoder double clicked - Switched to %s mode", 
                  pushToTalkMode ? "Push-to-Talk" : "Toggle Mute");
        
        // Flash LED to indicate mode change
        if (pushToTalkMode) {
            // Blue flash for Push-to-Talk mode
            getLedStrip().setColor(getLedStrip().colorBlue());
            delay(200);
            getLedStrip().clear();
            delay(200);
            getLedStrip().setColor(getLedStrip().colorBlue());
            delay(200);
        } else {
            // Purple flash for Toggle mode
            getLedStrip().setColor(255, 0, 255);
            delay(200);
            getLedStrip().clear();
            delay(200);
            getLedStrip().setColor(255, 0, 255);
            delay(200);
        }
        
        // Restore LED state based on call and mute status
        updateLedCallStatus();
    }
    else if (event == BUTTON_LONG_PRESSED) {
        LOG_INFO("Encoder long pressed - Activating Bluetooth pairing mode");
        
        // Activate Bluetooth pairing mode
        getBLEHandler().startAdvertising();
        
        // Flash blue LED to indicate pairing mode
        for (int i = 0; i < 5; i++) {
            getLedStrip().setColor(0, 0, 255); // Blue for BT pairing
            delay(100);
            getLedStrip().clear();
            delay(100);
        }
    }
}

// --- Touch Event Handler ---
void DeviceController::onTouchEvent(TouchEvent event) {
    if (event == TOUCH_PRESSED) {
        LOG_DEBUG("Touch sensor activated");
        touchPressed = true;
        
        // Only allow mute/unmute if there's an active call
        if (!callActive) {
            LOG_DEBUG("Touch ignored - no active call");
            return;
        }
        
        if (pushToTalkMode) {
            // In push-to-talk mode, unmute only while touching
            if (muteState) {
                muteState = false;
                LOG_DEBUG("Push-to-talk: Unmuting while touched");
                
                // Update LED to show unmuted (green)
                updateLedCallStatus();
                if (getBLEHandler().getConnectedClients() > 0) {
                    updateCallState(muteState, dropState);
                }
            }
        } else {
            // In toggle mode, toggle mute state on press
            toggleMute();
            LOG_DEBUG("Touch sensor toggled mute. Mute is now: %s", muteState ? "ON" : "OFF");
            
            // Update LED to show mute status (red if muted, green if unmuted)
            updateLedCallStatus();
            if (getBLEHandler().getConnectedClients() > 0) {
                updateCallState(muteState, dropState);
            }
        }
    }
    else if (event == TOUCH_RELEASED) {
        LOG_DEBUG("Touch sensor released");
        touchPressed = false;
        
        // Only allow mute/unmute if there's an active call
        if (!callActive) {
            LOG_DEBUG("Touch release ignored - no active call");
            return;
        }
        
        if (pushToTalkMode) {
            // In push-to-talk mode, mute when released
            muteState = true;
            LOG_DEBUG("Push-to-talk: Muting on release");
            
            // Update LED to show muted (red)
            updateLedCallStatus();
            if (getBLEHandler().getConnectedClients() > 0) {
                updateCallState(muteState, dropState);
            }
        }
    }
}

// --- Encoder Event Handler ---
void DeviceController::onEncoderEvent(EncoderEvent event) {
    // Encoder behavior is the same whether in a call or not
    // Switch between volume control and arrow keys based on encoder mode
    if (encoderVolumeMode) {
        // Volume control mode (inverted direction)
        switch (event) {
            case ENCODER_CLOCKWISE:
                // Volume down (inverted)
                LOG_DEBUG("Encoder rotated clockwise (volume mode): Volume Down");
                getKeyboardHandler().sendVolumeDown();
                break;
            case ENCODER_COUNTER_CLOCKWISE:
                // Volume up (inverted)
                LOG_DEBUG("Encoder rotated counter-clockwise (volume mode): Volume Up");
                getKeyboardHandler().sendVolumeUp();
                break;
            default:
                break;
        }
    } else {
        // Arrow key mode
        switch (event) {
            case ENCODER_CLOCKWISE:
                // Left Arrow key
                LOG_DEBUG("Encoder rotated clockwise (arrow mode): Sending Left Arrow");
                getKeyboardHandler().sendLeftArrow();
                break;
            case ENCODER_COUNTER_CLOCKWISE:
                // Right Arrow key
                LOG_DEBUG("Encoder rotated counter-clockwise (arrow mode): Sending Right Arrow");
                getKeyboardHandler().sendRightArrow();
                break;
            default:
                break;
        }
    }
}

void DeviceController::updateLedCallStatus() {
    // No call: LED OFF
    if (!callActive) {
        getLedStrip().clear();
        return;
    }
    
    // Call active: RED if muted, GREEN if not
    getLedStrip().setColor(
        muteState ?
        getLedStrip().colorRed() :
        getLedStrip().colorGreen()
    );
}

void DeviceController::onHostStateUpdate(bool hostCallActive, bool hostMuteState) {
    // Update internal state based on host (computer) updates
    callActive = hostCallActive;
    muteState = hostMuteState;
    
    // Update LED status to reflect the new state
    updateLedCallStatus();
    
    LOG_DEBUG("Host state updated - Call: %s, Mute: %s",
                  callActive ? "Active" : "Idle",
                  muteState ? "ON" : "OFF");
}

// --- Static Callback Functions ---
// These functions bridge the gap between C-style callbacks and instance methods

void DeviceController::staticLeftButtonCallback(ButtonEvent event) {
    if (instance) {
        instance->onLeftButtonEvent(event);
    }
}

void DeviceController::staticRightButtonCallback(ButtonEvent event) {
    if (instance) {
        instance->onRightButtonEvent(event);
    }
}

void DeviceController::staticEncoderButtonCallback(ButtonEvent event) {
    if (instance) {
        instance->onEncoderButtonEvent(event);
    }
}

void DeviceController::staticTouchCallback(TouchEvent event) {
    if (instance) {
        instance->onTouchEvent(event);
    }
}

void DeviceController::staticEncoderCallback(EncoderEvent event) {
    if (instance) {
        instance->onEncoderEvent(event);
    }
}

void DeviceController::staticHostStateCallback(bool callActive, bool muteState) {
    if (instance) {
        instance->onHostStateUpdate(callActive, muteState);
    }
}
