#pragma once
#include <BLE2902.h>

#include "BLEServiceManager.h"
#include "Config.h"
#include "utils.h"

const char BLE_MAC_ADDRESS_SERVICE_UUID[] =
    "709F0001-37E3-439E-A338-23F00067988B";
const char BLE_MAC_ADDRESS_CHAR_UUID[] = "709F0002-37E3-439E-A338-23F00067988B";
const char BLE_DEVICE_NAME_CHAR_UUID[] = "709F0003-37E3-439E-A338-23F00067988B";

class BLEDeviceNameCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *ch) {
    std::string deviceName = ch->getValue();
    Config::getInstance().setBLEDeviceName(deviceName);
    esp_err_t errRc = ::esp_ble_gap_set_device_name(deviceName.c_str());
    if (errRc != ESP_OK) {
      log_e("esp_ble_gap_set_device_name: rc=%d", errRc);
    }
    ch->setValue((uint8_t *)deviceName.data(), deviceName.length());
    ch->notify();
  }
};

class BLEMACAddressServiceHandler : public BLEServiceHandler {
 public:
  BLEMACAddressServiceHandler(BLEServiceManager &manager)
      : BLEServiceHandler(manager, BLE_MAC_ADDRESS_SERVICE_UUID) {
    auto *macaddressChar = bleService_->createCharacteristic(
        BLE_MAC_ADDRESS_CHAR_UUID, BLECharacteristic::PROPERTY_READ);
    macaddressChar->addDescriptor(new BLE2902());

    std::string macAddress = getMACAddress();
    Serial.printf("MAC address = %s\n", macAddress.c_str());
    macaddressChar->setValue((uint8_t *)macAddress.data(), macAddress.length());

    auto *deviceNameChar = bleService_->createCharacteristic(
        BLE_DEVICE_NAME_CHAR_UUID, BLECharacteristic::PROPERTY_READ |
                                       BLECharacteristic::PROPERTY_WRITE |
                                       BLECharacteristic::PROPERTY_NOTIFY);
    std::string deviceName = Config::getInstance().getBLEDeviceName("");
    Serial.printf("Device name = %s\n", deviceName.c_str());
    deviceNameChar->setValue((uint8_t *)deviceName.data(), deviceName.length());
    deviceNameChar->setCallbacks(new BLEDeviceNameCallbacks());
  }
};
