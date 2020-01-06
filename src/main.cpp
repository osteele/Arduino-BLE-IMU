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

static BLEServiceManager *bleServiceManager;

void setup() {
  // Serial.begin(115200);
  Serial.begin(9600);

  Wire.beginTransmission(0x28);
  byte error = Wire.endTransmission();
  BNO055Base *bno =
      error == 0 ? static_cast<BNO055Base *>(new BNO055Adaptor<Adafruit_BNO055>(
                       *new Adafruit_BNO055()))
                 : static_cast<BNO055Base *>(new BNO055Dummy());
  bno->begin();

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
