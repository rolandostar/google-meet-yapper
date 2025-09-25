#ifndef BLUETOOTH_HANDLER_H
#define BLUETOOTH_HANDLER_H

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "hidmap.h"
#include "config.h"

// Forward declarations
class MultiClientServerCallbacks;
class OutputCallbacks;

// Callback function type for host state updates
typedef void (*HostStateCallback)(bool callActive, bool muteState);

class BluetoothHandler {
public:
    BluetoothHandler();
    void begin();
    
    // Start BLE advertising for pairing
    void startAdvertising();
    
    // Generic method for sending reports to any characteristic
    bool sendReport(BLECharacteristic* characteristic, uint8_t* report, size_t length, bool notifyAll = true);
    
    // High-level report sending methods
    bool sendHeadsetReport(uint8_t reportValue);
    bool sendKeyboardReport(uint8_t* reportValue);
    bool sendConsumerReport(uint16_t consumerCode);
    
    // Status methods
    uint32_t getConnectedClients() const { return connectedClients; }
    bool isConnected() const { return connectedClients > 0; }

    bool isInitialized() const { return (headsetInput != nullptr && keyboardInput != nullptr && consumerInput != nullptr); }

    // Characteristic accessors
    BLECharacteristic* getHeadsetInputCharacteristic() { return headsetInput; }
    BLECharacteristic* getKeyboardInputCharacteristic() { return keyboardInput; }
    BLECharacteristic* getConsumerInputCharacteristic() { return consumerInput; }
    
    // Get HID device for keyboard handler
    BLEHIDDevice* getHIDDevice() const { return hid; }
    
    // Host state callback registration
    void setHostStateCallback(HostStateCallback callback) { hostStateCallback = callback; }

    // Singleton instance getter
    static BluetoothHandler& getInstance() {
        static BluetoothHandler instance;
        return instance;
    }

private:
    friend class MultiClientServerCallbacks; // Allow the callback to modify connectedClients
    friend class OutputCallbacks; // Allow the callback to access hostStateCallback
    friend void bluetoothTask(void*); // Allow the task to access private members

    uint32_t connectedClients;
    BLEHIDDevice* hid;
    BLECharacteristic* headsetInput;
    BLECharacteristic* headsetOutput;
    BLECharacteristic* keyboardInput;  // Added keyboard input characteristic
    BLECharacteristic* consumerInput;  // Added consumer control input characteristic
    BLEServer* pServer;
    HostStateCallback hostStateCallback = nullptr; // Callback for host state updates
    
    void initBLE();
};

/*
 * Callbacks for BLE Server connection events.
 */
class MultiClientServerCallbacks : public BLEServerCallbacks {
public:
    MultiClientServerCallbacks() {}
    
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

/*
 * Callback for handling output reports from host
 */
class OutputCallbacks : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic* pCharacteristic) override;
};

// Global accessor function
BluetoothHandler& getBLEHandler();

#endif // BLUETOOTH_HANDLER_H
