#include <BLE2902.h>
#include "BLEServiceHandler.h"

#include "Config.h"
#include "utils.h"

const char BLE_MAC_ADDRESS_SERVICE_UUID[] =
    "709F0001-37E3-439E-A338-23F00067988B";
const char BLE_MAC_ADDRESS_CHAR_UUID[] = "709F0002-37E3-439E-A338-23F00067988B";
const char BLE_DEVICE_NAME_CHAR_UUID[] = "709F0003-37E3-439E-A338-23F00067988B";

class BLEDeviceNameCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *ch) {
    std::string value = ch->getValue();
    Config::getInstance().setBLEDeviceName(value);
  }
};

class BLE_MACAddressServiceHandler : public BLEServiceHandler {
 public:
  BLE_MACAddressServiceHandler(BLEServer *bleServer)
      : BLEServiceHandler(bleServer, BLE_MAC_ADDRESS_SERVICE_UUID) {
    auto *macaddressChar = bleService_->createCharacteristic(
        BLE_MAC_ADDRESS_CHAR_UUID, BLECharacteristic::PROPERTY_READ);
    macaddressChar->addDescriptor(new BLE2902());

    std::string macAddress = getMACAddress();
    macaddressChar->setValue((uint8_t *)macAddress.data(), macAddress.length());

    auto *deviceNameChar = bleService_->createCharacteristic(
        BLE_DEVICE_NAME_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    deviceNameChar->setCallbacks(new BLEDeviceNameCallbacks());
  }
};
