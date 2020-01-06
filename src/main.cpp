#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>

#include "BLEIMUService.h"
#include "BLEMACAddressService.h"
#include "BLEServiceManager.h"
#include "BLEUARTService.h"
#include "BNO055Dummy.h"

static const char BLE_ADV_NAME[] = "ESP32 IMU";

static BLEServiceManager* bleServiceManager;

static BNO055Base* getBNO055() {
  // Wire.beginTransmission(0x28);
  // byte error = Wire.endTransmission();
  auto bno = new Adafruit_BNO055();
  if (bno->begin())
    return new BNO055Adaptor<Adafruit_BNO055>(*new Adafruit_BNO055());
  return new BNO055Dummy();
}

void setup() {
  Serial.begin(115200);

  BNO055Base* bno = getBNO055();

  BLEDevice::init(BLE_ADV_NAME);
  bleServiceManager = new BLEServiceManager();
  bleServiceManager->addServiceHandler(
      new BLE_IMUServiceHandler(bleServiceManager->bleServer, *bno), true);
  bleServiceManager->addServiceHandler(
      new BLE_MACAddressServiceHandler(bleServiceManager->bleServer));
  bleServiceManager->addServiceHandler(
      new BLE_UARTServiceHandler(bleServiceManager->bleServer));

  Serial.println("Starting BLE...");
  bleServiceManager->start();
}

void loop() { bleServiceManager->tick(); }
