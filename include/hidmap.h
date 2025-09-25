#ifndef HIDMAP_H
#define HIDMAP_H

#include <Arduino.h>
#include <tusb.h>

// --- HID Report ID ---
#define HID_REPORTID_PHONE_INPUT 0x01
#define HID_REPORTID_LED_OUTPUT  0x02
#define HID_REPORTID_KEYBOARD_INPUT 0x03
#define HID_REPORTID_CONSUMER_INPUT 0x04

// --- Consumer Control Usage Codes ---
#define CONSUMER_VOLUME_UP      0x01  // Bit 0
#define CONSUMER_VOLUME_DOWN    0x02  // Bit 1
#define CONSUMER_MUTE           0x04  // Bit 2

/*
 * This map defines:
 * 1. A headset that reports the Phone Mute and Drop states using telephony page
 * 2. Can receive mute commands from the host using the LED page
 * 3. Can send keyboard commands (arrow keys, key combinations)
 */
static const uint8_t REPORT_MAP[] = {
    // Telephony Collection
    0x05, 0x0b,                     // USAGE_PAGE (Telephony Devices)
    0x09, 0x05,                     // USAGE (Headset)
    0xa1, 0x01,                     // COLLECTION (Application)
    
    // Input report for sending mute and drop state to host
    0x85, HID_REPORTID_PHONE_INPUT, //   REPORT_ID (1)
    0x25, 0x01,                     //   LOGICAL_MAXIMUM (1)
    0x15, 0x00,                     //   LOGICAL_MINIMUM (0)
    0x09, 0x2f,                     //   USAGE (Phone Mute - 0x0B2F - 720943)
    0x09, 0x26,                     //   USAGE (Phone Drop - 0x0B26 - 720934)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x02,                     //   REPORT_COUNT (2)
    0x81, 0x02,                     //   INPUT (Data,Var,Abs)
    0x95, 0x06,                     //   REPORT_COUNT (6) - Padding
    0x81, 0x03,                     //   INPUT (Cnst,Var,Abs)
    
    0xc0,                           // END_COLLECTION
    
    // LED Output Collection for receiving mute and hook commands
    0x05, 0x08,                     // USAGE_PAGE (LEDs)
    0x09, 0x01,                     // USAGE (LED Indicator)
    0xa1, 0x01,                     // COLLECTION (Application)
    
    // Output report for receiving mute and hook commands from host
    0x85, HID_REPORTID_LED_OUTPUT,  //   REPORT_ID (2)
    0x09, 0x09,                     //   USAGE (Mute - 0x0809 - 524297)
    0x09, 0x17,                     //   USAGE (Off-Hook - 0x0817 - 524311)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x02,                     //   REPORT_COUNT (2)
    0x91, 0x02,                     //   OUTPUT (Data,Var,Abs)
    0x95, 0x06,                     //   REPORT_COUNT (6) - Padding
    0x91, 0x03,                     //   OUTPUT (Cnst,Var,Abs)
    
    0xc0,                           // END_COLLECTION
    
    // ------------------ Keyboard Collection ------------------
    USAGE_PAGE(1),      0x01, // Generic Desktop
    USAGE(1),           0x06, // Keyboard
    COLLECTION(1),      0x01, // Application
      REPORT_ID(1),       0x03, //   Report ID (1)
      USAGE_PAGE(1),      0x07, //   Keyboard/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01, //   1 bit
      REPORT_COUNT(1),    0x08, //   8 bits
      HIDINPUT(1),           0x02, //   Data, Var, Abs (Modifier keys)
      REPORT_COUNT(1),    0x01, //   1 report
      REPORT_SIZE(1),     0x08, //   8 bits
      HIDINPUT(1),           0x01, //   Const, Array, Abs (Reserved byte)
      REPORT_COUNT(1),    0x05, //   5 reports
      REPORT_SIZE(1),     0x01, //   1 bit
      USAGE_PAGE(1),      0x08, //   LEDs
      USAGE_MINIMUM(1),   0x01, //   Num Lock
      USAGE_MAXIMUM(1),   0x05, //   Scroll Lock
      HIDOUTPUT(1),          0x02, //   Data, Var, Abs (LED states)
      REPORT_COUNT(1),    0x01, //   1 report
      REPORT_SIZE(1),     0x03, //   3 bits
      HIDOUTPUT(1),          0x01, //   Const, Array, Abs (LED padding)
      REPORT_COUNT(1),    0x06, //   6 reports
      REPORT_SIZE(1),     0x08, //   8 bits
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65, //   101 keys
      USAGE_PAGE(1),      0x07, //   Keyboard/Keypad
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00, //   Data, Array, Abs (Keycodes)
    END_COLLECTION(0),
    
    // ------------------ Consumer Control Collection ------------------
    USAGE_PAGE(1),      0x0C, // Consumer Devices
    USAGE(1),           0x01, // Consumer Control
    COLLECTION(1),      0x01, // Application
      REPORT_ID(1),       0x04, //   Report ID (4)
      USAGE(1),           0xE9, //   Usage (Volume Increment)
      USAGE(1),           0xEA, //   Usage (Volume Decrement)
      USAGE(1),           0xE2, //   Usage (Mute)
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,
      REPORT_COUNT(1),    0x03,
      HIDINPUT(1),        0x02, //   Data, Array, Abs (Consumer control codes)
      REPORT_COUNT(1),    0x05,
      HIDINPUT(1),        0x01, //   Data, Array, Abs (Consumer control codes)
    END_COLLECTION(0)
};

#endif // HIDMAP_H
