#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>

#include "BLEIMUService.h"
#include "BLEMACAddressService.h"
#include "BLEServiceManager.h"
#include "BLEUARTService.h"
#include "BNO055Dummy.h"
#include "Config.h"

static const char BLE_ADV_NAME[] = "ESP32 IMU";

static BLEServiceManager* bleServiceManager;

static BNO055Base* getBNO055() {
  Wire.begin(23, 22);
  // pinMode(22, INPUT_PULLUP);
  // pinMode(23, INPUT_PULLUP);
  // Serial.print("I2C scan -> [");
  // int count = 0;
  // for (int i = 0; i < 255; i++) {
  //   Wire.beginTransmission(0x28);
  //   byte error = Wire.endTransmission();
  //   if (error == 0) {
  //     if (count++ > 0) Serial.print(",");
  //     Serial.print(i);
  //   }
  // }
  // Serial.println("]");

  auto bno = new Adafruit_BNO055();
  if (bno->begin()) {
    Serial.println("Connected to BNO055");
    auto bno = *new Adafruit_BNO055();
    bno.printSensorDetails();
    return new BNO055Adaptor<Adafruit_BNO055>(bno);
  } else {
    Serial.println("Simulating BNO055");
    return new BNO055Dummy();
  }
}

void setup() {
  Serial.begin(115200);

  std::string bleDeviceName =
      Config::getInstance().getBLEDeviceName(BLE_ADV_NAME);
  BNO055Base* bno = getBNO055();

  BLEDevice::init(bleDeviceName.c_str());
  bleServiceManager = new BLEServiceManager();
  bleServiceManager->addServiceHandler(
      new BLE_IMUServiceHandler(bleServiceManager->bleServer, *bno), true);
  bleServiceManager->addServiceHandler(
      new BLE_MACAddressServiceHandler(bleServiceManager->bleServer));
  bleServiceManager->addServiceHandler(
      new BLE_UARTServiceHandler(bleServiceManager->bleServer));

  Serial.print("Starting BLE (device name=");
  Serial.print(bleDeviceName.c_str());
  Serial.println(")");
  bleServiceManager->start();
}

void loop() { bleServiceManager->tick(); }
