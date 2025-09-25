#pragma once

#include <Arduino.h>
#include "hardware/button.h"
#include "hardware/touch_sensor.h"
#include "hardware/rotary_encoder.h"

/**
 * @brief Main controller class that manages all device functionality
 * 
 * This class coordinates between hardware components (buttons, sensors, etc.)
 * and communication protocols (Bluetooth, Serial, Keyboard) to provide
 * telephony control functionality and maintains all device state.
 */
class DeviceController {
private:
    // Hardware components
    Button leftButton;
    Button rightButton;
    
    // Device state variables
    bool muteState = false;
    bool dropState = false;
    bool callActive = false;
    bool pushToTalkMode = false;
    bool touchPressed = false;
    bool encoderVolumeMode = true;  // true = volume control, false = arrow keys
    
    // Static instance pointer for callbacks
    static DeviceController* instance;

public:
    DeviceController();
    
    /**
     * @brief Initialize all hardware components and communication protocols
     */
    void begin();
    
    /**
     * @brief Update all components - should be called in main loop
     */
    void update();
    
    /**
     * @brief Update call state for all connected clients
     * @param muteValue Mute state to send
     * @param dropValue Drop call state to send
     */
    void updateCallState(bool muteValue, bool dropValue);
    
    // State getters
    bool isMuted() const { return muteState; }
    bool isDropped() const { return dropState; }
    bool isCallActive() const { return callActive; }
    bool isPushToTalkMode() const { return pushToTalkMode; }
    bool isTouchPressed() const { return touchPressed; }
    bool isEncoderVolumeMode() const { return encoderVolumeMode; }

    // State setters
    void setMute(bool state) { muteState = state; }
    void setDrop(bool state) { dropState = state; }
    void setCallActive(bool state) { callActive = state; }
    void setPushToTalkMode(bool mode) { pushToTalkMode = mode; }
    void setTouchPressed(bool pressed) { touchPressed = pressed; }
    void setEncoderVolumeMode(bool mode) { encoderVolumeMode = mode; }

    // Convenience methods
    void toggleMute() { muteState = !muteState; }
    void togglePushToTalk() { pushToTalkMode = !pushToTalkMode; }
    void toggleEncoderMode() { encoderVolumeMode = !encoderVolumeMode; }
    
    /**
     * @brief Handle host state updates from Bluetooth
     * @param callActive Whether a call is active
     * @param muteState Whether the call should be muted
     */
    void onHostStateUpdate(bool callActive, bool muteState);

private:
    // Event handlers (instance methods)
    void onLeftButtonEvent(ButtonEvent event);
    void onRightButtonEvent(ButtonEvent event);
    void onEncoderButtonEvent(ButtonEvent event);
    void onTouchEvent(TouchEvent event);
    void onEncoderEvent(EncoderEvent event);
    void updateLedCallStatus();
    
    // Static callback functions for hardware (C-style callbacks)
    static void staticLeftButtonCallback(ButtonEvent event);
    static void staticRightButtonCallback(ButtonEvent event);
    static void staticEncoderButtonCallback(ButtonEvent event);
    static void staticTouchCallback(TouchEvent event);
    static void staticEncoderCallback(EncoderEvent event);
    
    // Static callback for host state updates
    static void staticHostStateCallback(bool callActive, bool muteState);
};
