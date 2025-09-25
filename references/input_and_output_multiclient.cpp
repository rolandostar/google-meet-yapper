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
#define DEVICE_VID          0xFEED // Vendor ID for hobbyist projects
#define DEVICE_PID          0xC0DE // Product ID
#define DEVICE_VERSION      0x0100 // Version 1.00

// --- Configuration ---
#define MUTE_BUTTON_PIN   0   // Press to toggle mute
#define LED_PIN           2   // Lights up when mute is active
// #define HANGUP_BUTTON_PIN 5 // No longer used in this report map

// --- HID Report ID ---
#define HID_REPORTID_PHONE_INPUT 0x01
#define HID_REPORTID_LED_OUTPUT  0x02

// --- Bluetooth Appearance ---
#define HID_HEADSET 0x0941 // Standard BLE appearance for a headset

// --- Global Variables ---
uint32_t connectedClients = 0;
bool muteState = false; // The device's internal mute state

// Button state tracking for debouncing
int previousMuteButtonState = HIGH;

// BLE HID objects
BLEHIDDevice* hid;
BLECharacteristic* headsetInput;
BLECharacteristic* headsetOutput;  // New output characteristic

// Forward declarations
void bluetoothTask(void*);
void sendMuteReport();

/*
 * ** UPDATED: HID Report Map with Telephony Input and LED Output **
 * This map defines a headset that reports the Phone Mute state using telephony page
 * and can receive mute commands from the host using the LED page.
 */
static const uint8_t REPORT_MAP[] = {
    0x05, 0x0b,                     // USAGE_PAGE (Telephony Devices)
    0x09, 0x05,                     // USAGE (Headset)
    0xa1, 0x01,                     // COLLECTION (Application)
    
    // Input report for sending mute state to host
    0x85, HID_REPORTID_PHONE_INPUT, //   REPORT_ID (1)
    0x25, 0x01,                     //   LOGICAL_MAXIMUM (1)
    0x15, 0x00,                     //   LOGICAL_MINIMUM (0)
    0x09, 0x2f,                     //   USAGE (Phone Mute)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x01,                     //   REPORT_COUNT (1)
    0x81, 0x02,                     //   INPUT (Data,Var,Abs)
    0x95, 0x07,                     //   REPORT_COUNT (7) - Padding
    0x81, 0x03,                     //   INPUT (Cnst,Var,Abs)
    
    0xc0,                           // END_COLLECTION
    
    // LED Output Collection for receiving mute commands
    0x05, 0x08,                     // USAGE_PAGE (LEDs)
    0x09, 0x01,                     // USAGE (LED Indicator)
    0xa1, 0x01,                     // COLLECTION (Application)
    
    // Output report for receiving mute commands from host
    0x85, HID_REPORTID_LED_OUTPUT,  //   REPORT_ID (2)
    0x09, 0x09,                     //   USAGE (Mute)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x01,                     //   REPORT_COUNT (1)
    0x91, 0x02,                     //   OUTPUT (Data,Var,Abs)
    0x95, 0x07,                     //   REPORT_COUNT (7) - Padding
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
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("Received output report: ");
            for(int i = 0; i < value.length(); i++) {
                Serial.printf("0x%.2X ", (uint8_t)value[i]);
            }
            Serial.println();
            
            // Make sure we have at least the report ID and one data byte
            if (value.length() >= 2) {
                uint8_t reportId = (uint8_t)value[0];
                uint8_t muteCommand = (uint8_t)value[1] & 0x01; // Extract just the mute bit
                
                Serial.printf("Report ID: %d, Mute command: %d\n", reportId, muteCommand);
                
                // Only update if the state has changed
                if (muteState != (muteCommand == 1)) {
                    muteState = (muteCommand == 1);
                    Serial.printf("Received mute command from host. Mute is now: %s\n", 
                                muteState ? "ON" : "OFF");
                    
                    // Update LED to reflect the new state
                    digitalWrite(LED_PIN, muteState ? HIGH : LOW);
                    
                    // Broadcast the state to all connected clients to ensure synchronization
                    sendMuteReport();
                }
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 Mute-Only Telephony Controller...");

    pinMode(MUTE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Start with LED off

    xTaskCreate(bluetoothTask, "bluetooth", 5000, NULL, 5, NULL);
}

void loop() {
    // --- Handle Mute Button ---
    int currentMuteButtonState = digitalRead(MUTE_BUTTON_PIN);
    if (currentMuteButtonState == LOW && previousMuteButtonState == HIGH) {
        muteState = !muteState; // Toggle the internal mute state
        Serial.printf("Button pressed. Mute is now: %s\n", muteState ? "ON" : "OFF");

        digitalWrite(LED_PIN, muteState ? HIGH : LOW); // Update LED immediately

        if (connectedClients > 0) {
            sendMuteReport(); // Send the new state to all connected clients
        }
    }
    previousMuteButtonState = currentMuteButtonState;

    delay(50); // Debounce delay
}

/*
 * Sends the current mute state to ALL connected clients.
 */
void sendMuteReport() {
    if (connectedClients == 0 || !headsetInput) return;

    // The report is a single byte where bit 0 is the mute state.
    uint8_t reportValue = muteState ? 1 : 0;
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
