/*
 * ESP32 USB HID Telephony Controller
 * Modified 24 Sep 2025
 * by Rolando Romero
 * 
 * Simple telephony controller that sends mute and call drop commands over USB HID.
 * - GPIO0: Mute toggle button
 * - GPIO2: Call drop toggle button
 * 
 * Based on the ESP32 USB HID library and telephony HID usage page.
 * 
 * USB Mode in Arduino IDE MUST BE ENABLED
 * USB Mode: "USB-OTG (TinyUSB)" 
 * USB CDC On Boot: Enabled
 * USB Firmware MSC On Boot: Disabled
 * USB DFU On Boot: Disabled
 * Upload Mode: "USB-OTG CDC (TinyUSB)"
 * 
 * This example code is in the public domain.
 */

#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include "USB.h"
#include "USBHID.h"

USBHID HID;

// Pin definitions
const int MUTE_BUTTON_PIN = 0;   // GPIO0 for mute toggle
const int DROP_BUTTON_PIN = 2;   // GPIO2 for call drop toggle

// Button state tracking
bool muteState = false;
bool dropState = false;

// Complete telephony HID report descriptor with input and output reports
static const uint8_t telephony_report_descriptor[] = {
  0x05, 0x0B,        // Usage Page (Telephony Devices)
  0x09, 0x05,        // Usage (Headset)
  0xA1, 0x01,        // Collection (Application)
  
  // Input report for sending mute and drop states to host
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x09, 0x2F,        //   Usage (Phone Mute)
  0x09, 0x26,        //   Usage (Phone Drop)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  
  // Padding bits (6 bits to make a full byte)
  0x95, 0x06,        //   Report Count (6)
  0x81, 0x03,        //   Input (Cnst,Var,Abs)
  
  0xC0,              // End Collection
  
  // LED Output Collection for receiving commands from host
  0x05, 0x08,        // Usage Page (LEDs)
  0x09, 0x01,        // Usage (LED Indicator)
  0xA1, 0x01,        // Collection (Application)
  
  // Output report for receiving mute and hook commands from host
  0x85, 0x02,        //   Report ID (2)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x09, 0x09,        //   Usage (Mute LED)
  0x09, 0x17,        //   Usage (Off-Hook LED)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x02,        //   Report Count (2)
  0x91, 0x02,        //   Output (Data,Var,Abs)
  
  // Padding bits (6 bits to make a full byte)
  0x95, 0x06,        //   Report Count (6)
  0x91, 0x03,        //   Output (Cnst,Var,Abs)
  
  0xC0               // End Collection
};

class TelephonyHIDDevice : public USBHIDDevice {
public:
  TelephonyHIDDevice(void) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
      HID.addDevice(this, sizeof(telephony_report_descriptor));
    }
  }

  void begin(void) {
    HID.begin();
  }

  uint16_t _onGetDescriptor(uint8_t *buffer) {
    memcpy(buffer, telephony_report_descriptor, sizeof(telephony_report_descriptor));
    return sizeof(telephony_report_descriptor);
  }

  bool sendTelephonyReport(bool mute, bool drop) {
    uint8_t report = (mute ? 0x01 : 0x00) | (drop ? 0x02 : 0x00);
    return HID.SendReport(1, &report, 1);  // Report ID 1, 1 byte of data
  }

  // Handle output reports from host
  void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) {
    if (report_id == 2 && len >= 1) { // Report ID 2 for LED output
      bool hostMuteState = buffer[0] & 0x01;
      bool hostOffHookState = buffer[0] & 0x02;
      
      Serial0.printf("Host command - Mute: %s, Off-Hook: %s\n", 
                   hostMuteState ? "ON" : "OFF", 
                   hostOffHookState ? "ON" : "OFF");
      
      // You can choose to sync your internal state with host commands
      // For example, update muteState and dropState based on host commands:
      // muteState = hostMuteState;
      // dropState = hostOffHookState;
    }
  }
};

TelephonyHIDDevice telephonyDevice;

void setup() {
  Serial0.begin(115200);
  Serial0.println("ESP32 USB Telephony Controller Starting...");
  
  // Initialize button pins
  pinMode(MUTE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DROP_BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize USB HID
  telephonyDevice.begin();
  USB.begin();
  
  Serial0.println("Ready! Press GPIO0 for mute, GPIO2 for call drop");
}

void loop() {
  // Handle mute button (GPIO0)
  if (digitalRead(MUTE_BUTTON_PIN) == LOW) {
    muteState = !muteState;
    Serial0.printf("Mute toggled: %s\n", muteState ? "ON" : "OFF");
    
    if (HID.ready()) {
      telephonyDevice.sendTelephonyReport(muteState, dropState);
    }
    delay(300); // Simple delay to prevent multiple toggles
  }
  
  // Handle drop button (GPIO2)
  if (digitalRead(DROP_BUTTON_PIN) == LOW) {
    dropState = !dropState;
    Serial0.printf("Call drop toggled: %s\n", dropState ? "ACTIVE" : "INACTIVE");
    
    if (HID.ready()) {
      telephonyDevice.sendTelephonyReport(muteState, dropState);
    }
    delay(300); // Simple delay to prevent multiple toggles
  }
  
  delay(50); // Small delay for main loop
}

#endif /* ARDUINO_USB_MODE */