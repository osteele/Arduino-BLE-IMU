#include <BLE2902.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <WiFi.h>

const char BLE_MAC_ADDRESS_SERVICE_UUID[] =
    "709F0001-37E3-439E-A338-23F00067988B";
const char BLE_MAC_ADDRESS_CHAR_UUID[] = "709F0002-37E3-439E-A338-23F00067988B";

class BLEMACAddressServiceHandler {
 public:
  BLEMACAddressServiceHandler(BLEServer *bleServer) {
    bleService = bleServer->createService(BLE_MAC_ADDRESS_SERVICE_UUID);
    auto *bleChar = bleService->createCharacteristic(
        BLE_MAC_ADDRESS_CHAR_UUID, BLECharacteristic::PROPERTY_READ);
    bleChar->addDescriptor(new BLE2902());

    byte macAddress[6];
    WiFi.macAddress(macAddress);
    uint8_t macString[2 * sizeof macAddress + 1];
    BLEUtils().buildHexData(macString, macAddress, 6);
    bleChar->setValue(macString, 2 * sizeof macAddress);
  }

  void start() { bleService->start(); }

 private:
  BLEService *bleService;
};
