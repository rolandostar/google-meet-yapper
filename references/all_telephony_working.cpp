/*
 * ESP32 acting as a Multi-Client Bluetooth Telephony (Mute-Only) Controller
 *
 * This program allows an ESP32 to broadcast its mute state to multiple
 * connected host devices simultaneously over BLE HID using a simplified
 * Telephony Device Page report map.
 *
 * This version only reports the mute status and does not include a hook switch
 * or receive commands from the host. The device is the sole source of truth
 * for the mute state.
 *
 * NOTE: This is a non-standard implementation of the BLE HID protocol.
 * Standard HID is a one-to-one connection. This implementation allows
 * multiple hosts to connect to a single peripheral.
 */

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"

// --- Custom Device Information ---
#define DEVICE_MANUFACTURER "Custom Gadgets Inc."
#define DEVICE_NAME         "ESP32 Mute Control"
#define DEVICE_VID          0xEDFE // Vendor ID for hobbyist projects
#define DEVICE_PID          0xDEC0 // Product ID
#define DEVICE_VERSION      0x0100 // Version 1.00

// --- Configuration ---
#define MUTE_BUTTON_PIN   0   // Press to toggle mute
#define LED_PIN           2   // Lights up when mute is active
#define DROP_BUTTON_PIN   17  // Press to toggle drop call state (formerly hook button)
// #define HANGUP_BUTTON_PIN 5 // No longer used in this report map

// --- HID Report ID ---
#define HID_REPORTID_PHONE_INPUT 0x01
#define HID_REPORTID_LED_OUTPUT  0x02

// --- Bluetooth Appearance ---
#define HID_HEADSET 0x0941 // Standard BLE appearance for a headset

// --- Global Variables ---
uint32_t connectedClients = 0;
bool muteState = false; // The device's internal mute state
bool dropState = false; // The device's internal drop call state (formerly hook state)

// Button state tracking for debouncing
int previousMuteButtonState = HIGH;
int previousDropButtonState = HIGH;

// BLE HID objects
BLEHIDDevice* hid;
BLECharacteristic* headsetInput;
BLECharacteristic* headsetOutput;  // New output characteristic

// Forward declarations
void bluetoothTask(void*);
void sendMuteReport();
void sendDropReport(); // Updated function declaration

/*
 * ** UPDATED: HID Report Map with Telephony Input and LED Output **
 * This map defines a headset that reports the Phone Mute and Drop states using telephony page
 * and can receive mute commands from the host using the LED page.
 */
static const uint8_t REPORT_MAP[] = {
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
    0x85, HID_REPORTID_LED_OUTPUT,  //   REPORT_ID (3)
    0x09, 0x09,                     //   USAGE (Mute - 0x0809 - 524297)
    0x09, 0x17,                     //   USAGE (Off-Hook - 0x0817 - 524311)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x02,                     //   REPORT_COUNT (2)
    0x91, 0x02,                     //   OUTPUT (Data,Var,Abs)
    0x95, 0x06,                     //   REPORT_COUNT (6) - Padding
    0x91, 0x03,                     //   OUTPUT (Cnst,Var,Abs)
    
    0xc0                            // END_COLLECTION
};

/*
 * Callbacks for BLE Server connection events.
 */
class MultiClientServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        connectedClients++;
        Serial.printf("Client connected. Total clients: %d\n", connectedClients);
        pServer->getAdvertising()->start();
        // Send the current state to the newly connected client
        sendMuteReport();
    };

    void onDisconnect(BLEServer* pServer) {
        connectedClients--;
        Serial.printf("Client disconnected. Total clients: %d\n", connectedClients);
    }
};

/*
 * Callback for handling output reports from host
 */
class OutputCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    Serial.print("Received packet from host, characteristic UUID: ");
    Serial.println(pCharacteristic->getUUID().toString().c_str());
    Serial.print("Data length: ");
    Serial.println(pCharacteristic->getValue().length());
    
    std::string value = pCharacteristic->getValue();
    Serial.print("Data (hex): ");
    for (int i = 0; i < value.length(); i++) {
      Serial.printf("%02X ", value[i]);
    }
    Serial.println();
    
    // Process LED commands from host if they are the right length
    if (value.length() > 0) {
      uint8_t reportData = value[0];
      bool ledMuteState = reportData & 0x01;
      bool ledOffHookState = reportData & 0x02;
      
      Serial.printf("LED Mute state: %s, LED Off-Hook state: %s\n", 
                   ledMuteState ? "ON" : "OFF", 
                   ledOffHookState ? "ON" : "OFF");
      
      // You can choose to update your internal state based on these commands
      // This example only logs the values but doesn't change internal state
    }
  }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 Telephony Controller...");

    pinMode(MUTE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DROP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Start with LED off

    xTaskCreate(bluetoothTask, "bluetooth", 5000, NULL, 5, NULL);
}

void loop() {
    // --- Handle Mute Button ---
    int currentMuteButtonState = digitalRead(MUTE_BUTTON_PIN);
    if (currentMuteButtonState == LOW && previousMuteButtonState == HIGH) {
        muteState = !muteState; // Toggle the internal mute state
        Serial.printf("Mute button pressed. Mute is now: %s\n", muteState ? "ON" : "OFF");

        digitalWrite(LED_PIN, muteState ? HIGH : LOW); // Update LED immediately

        if (connectedClients > 0) {
            sendMuteReport(); // Send the new state to all connected clients
        }
    }
    previousMuteButtonState = currentMuteButtonState;

    // --- Handle Drop Button ---
    int currentDropButtonState = digitalRead(DROP_BUTTON_PIN);
    if (currentDropButtonState == LOW && previousDropButtonState == HIGH) {
        dropState = !dropState; // Toggle the internal drop state
        Serial.printf("Drop button pressed. Drop is now: %s\n", dropState ? "ACTIVE" : "INACTIVE");

        if (connectedClients > 0) {
            sendDropReport(); // Send the new drop state to all connected clients
        }
    }
    previousDropButtonState = currentDropButtonState;

    delay(50); // Debounce delay
}

/*
 * Sends the current mute state to ALL connected clients.
 */
void sendMuteReport() {
    if (connectedClients == 0 || !headsetInput) return;

    // The report is a single byte with bit 0 as mute state and bit 1 as drop state
    uint8_t reportValue = (muteState ? 0x01 : 0x00) | (dropState ? 0x02 : 0x00);
    headsetInput->setValue(&reportValue, 1);
    headsetInput->notify(true); // Notify all subscribed clients
}

/*
 * Sends the current drop state to ALL connected clients.
 */
void sendDropReport() {
    if (connectedClients == 0 || !headsetInput) return;

    // The report is a single byte with bit 0 as mute state and bit 1 as drop state
    uint8_t reportValue = (muteState ? 0x01 : 0x00) | (dropState ? 0x02 : 0x00);
    headsetInput->setValue(&reportValue, 1);
    headsetInput->notify(true); // Notify all subscribed clients
}

void bluetoothTask(void*) {
    BLEDevice::init(DEVICE_NAME);
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MultiClientServerCallbacks());

    hid = new BLEHIDDevice(pServer);
    headsetInput = hid->inputReport(HID_REPORTID_PHONE_INPUT);
    
    // Initialize output report with new report ID and set callback
    headsetOutput = hid->outputReport(HID_REPORTID_LED_OUTPUT);
    headsetOutput->setCallbacks(new OutputCallbacks());

    hid->manufacturer()->setValue(DEVICE_MANUFACTURER);
    hid->pnp(0x02, DEVICE_VID, DEVICE_PID, DEVICE_VERSION);
    hid->hidInfo(0x00, 0x01);

    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
    hid->startServices();

    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_HEADSET);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    hid->setBatteryLevel(100);

    Serial.println("Bluetooth task running. Ready for multiple connections.");
    vTaskDelete(NULL);
}
