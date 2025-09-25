#include "communication/bluetooth_handler.h"
#include "communication/keyboard_handler.h"
#include "hardware/led_strip.h"
#include "config.h"

// Forward declaration for the task function
void bluetoothTask(void* pvParameters);

// Static accessor function
BluetoothHandler& getBLEHandler() {
    return BluetoothHandler::getInstance();
}

BluetoothHandler::BluetoothHandler() 
    : connectedClients(0), hid(nullptr), headsetInput(nullptr), headsetOutput(nullptr), keyboardInput(nullptr), consumerInput(nullptr), pServer(nullptr) {
}

void BluetoothHandler::begin() {
    xTaskCreate(bluetoothTask, "bluetooth", 5000, NULL, 5, NULL);
}

void BluetoothHandler::initBLE() {
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MultiClientServerCallbacks());

    hid = new BLEHIDDevice(pServer);
    headsetInput = hid->inputReport(HID_REPORTID_PHONE_INPUT);
    keyboardInput = hid->inputReport(HID_REPORTID_KEYBOARD_INPUT);
    consumerInput = hid->inputReport(HID_REPORTID_CONSUMER_INPUT);
    
    // Initialize output report with report ID and set callback
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
    // Change the appearance to a keyboard+pointer device
    pAdvertising->setAppearance(0x03C0);  // Keyboard/pointer HID
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    LOG_INFO("BLE Initialized: %s", DEVICE_NAME);
}

bool BluetoothHandler::sendReport(BLECharacteristic* characteristic, uint8_t* report, size_t length, bool notifyAll) {
  if (connectedClients == 0 || !characteristic) return false;

  LOG_DEBUG("Report data (hex, length=%d): ", length);
  char buffer[length * 3 + 1]; // 2 hex chars + space for each byte + null terminator
  int bufferIndex = 0;
  for (int i = 0; i < length; i++) {
    sprintf(buffer + bufferIndex, "%02X ", report[i]);
    bufferIndex += 3;
  }
  buffer[bufferIndex] = '\0'; // Null-terminate the string
  LOG_DEBUG("%s", buffer);

  // Set the report value
  characteristic->setValue(report, length);

  // Notify clients
  characteristic->notify(notifyAll);
  
  // The ESP32 BLE notify() returns void, so we just return success if we got this far
  return true;
}

bool BluetoothHandler::sendHeadsetReport(uint8_t reportValue) {
  if (!headsetInput) {
    LOG_ERROR("Headset input not initialized.");
    return false;
  }
  if (connectedClients == 0) {
    LOG_WARN("No connected clients to send headset report.");
    return false; 
  }
  bool success = sendReport(headsetInput, &reportValue, 1);
  if (!success) {
    LOG_ERROR("Failed to send headset report!");
  }
  return success;
}

bool BluetoothHandler::sendKeyboardReport(uint8_t* reportValue) {
  if (!keyboardInput) {
    LOG_ERROR("Keyboard input not initialized.");
    return false;
  }
  if (connectedClients == 0) {
    LOG_WARN("No connected clients to send keyboard report.");
    return false; 
  }
  // Silently fail if keyboard input is not initialized
  bool success = sendReport(keyboardInput, reportValue, 8);
  if (!success) {
    LOG_ERROR("Failed to send keyboard report!");
  }
  return success;
}

bool BluetoothHandler::sendConsumerReport(uint16_t consumerCode) {
  if (!consumerInput) {
    LOG_ERROR("Consumer input not initialized.");
    return false;
  }
  if (connectedClients == 0) {
    LOG_WARN("No connected clients to send consumer report.");
    return false; 
  }
  
  // Consumer report is 1 byte as per the HID descriptor
  uint8_t report = (uint8_t)(consumerCode & 0xFF);
  
  bool success = sendReport(consumerInput, &report, 1);
  if (!success) {
    LOG_ERROR("Failed to send consumer report!");
  }
  return success;
}

void BluetoothHandler::startAdvertising() {
  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  if (!pServer || !pAdvertising) {
    LOG_ERROR("BLE Server or Advertising not initialized");
    return;
  }

  pAdvertising->start();
}

void bluetoothTask(void* pvParameters) {
    BluetoothHandler& handler = BluetoothHandler::getInstance();
    handler.initBLE();
    vTaskDelete(NULL);
}

// MultiClientServerCallbacks implementation
void MultiClientServerCallbacks::onConnect(BLEServer* pServer) {
    BluetoothHandler& handler = BluetoothHandler::getInstance();
    handler.connectedClients++;
    LOG_INFO("BLE Client connected. Total clients: %d", handler.connectedClients);

    // Workaround for Windows and other devices that don't register for notifications
    // when reconnecting to a previously paired device
    auto enableNotifications = [&](BLECharacteristic* characteristic) {
      if (characteristic) {
        BLEDescriptor* desc = characteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        if (desc) {
          uint8_t val[] = {0x01, 0x00};
          desc->setValue(val, 2);
        }
      }
    };

    enableNotifications(handler.headsetInput);
    enableNotifications(handler.keyboardInput);
    enableNotifications(handler.consumerInput);
    
    // pServer->getAdvertising()->start();
}

void MultiClientServerCallbacks::onDisconnect(BLEServer* pServer) {
    BluetoothHandler& handler = BluetoothHandler::getInstance();
    handler.connectedClients--;
    LOG_INFO("Client disconnected. Total clients: %d", handler.connectedClients);
}

// OutputCallbacks implementation
void OutputCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    // Serial.print("Received packet from host, characteristic UUID: ");
    // Serial.println(pCharacteristic->getUUID().toString().c_str());
    // Serial.print("Data length: ");
    // Serial.println(pCharacteristic->getValue().length());
    
    std::string value = pCharacteristic->getValue();
    // Serial.print("Data (hex): ");
    // for (int i = 0; i < value.length(); i++) {
    //   Serial.printf("%02X ", value[i]);
    // }
    // Serial.println();
    
    // Process LED commands from host if they are the right length
    if (value.length() > 0) {
      uint8_t reportData = value[0];
      bool ledMuteState = reportData & 0x01;
      bool ledOffHookState = reportData & 0x02;
      
      LOG_DEBUG("Host state: Call %s, %s", 
        ledOffHookState ? "ACTIVE" : "IDLE",
        ledMuteState ? "MUTED" : "UNMUTED");

      // Use callback to update device controller state
      BluetoothHandler& handler = BluetoothHandler::getInstance();
      if (handler.hostStateCallback) {
          handler.hostStateCallback(ledOffHookState, ledMuteState);
      }
    }
}
