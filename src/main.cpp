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
static Adafruit_BNO055 bno055;

static BNO055Base* getBNO055() {
  Wire.begin(23, 22);

  if (bno055.begin()) {
    Serial.println("Connected to BNO055");
    bno055.printSensorDetails();
    return new BNO055Adaptor<Adafruit_BNO055>(bno055);
  } else {
    Serial.println("Simulating BNO055");
    return new BNO055Dummy();
  }
}

static void wifiConnect() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Unable to mount SPIFFS");
    return;
  }
  // if (!SPIFFS.exists("/wpa_supplicant.txt")) {
  //   Serial.println("File does not exist");
  //   return;
  // }
  File file = SPIFFS.open("/wpa_supplicant.txt");
  if (!file) {
    Serial.println("Unable to open file");
    return;
  }
  std::string ssid, password;
  int fieldNo = 0;
  while (file.available()) {
    int c = file.read();

    if (c == '\n') {
      if (++fieldNo > 1) break;
    } else {
      (fieldNo == 0 ? ssid : password).append(1, c);
    }
  }
  Serial.print("Connecting to WiFi ");
  Serial.print(ssid.c_str());
  Serial.print("...");
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("done");
  } else {
    Serial.print("\nWiFi connection failed. Status=");
    Serial.println(WiFi.status());
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

  wifiConnect();
}

void loop() { bleServiceManager->tick(); }
