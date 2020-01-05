#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include "BLE_IMU_Service.h"
#include "BLE_UART_Service.h"
#include "BNO055_Dummy.h"

static const char BLE_ADV_NAME[] = "ESP32 IMU";

static const int TX_DELAY = 900 / 60;  // 60 fps, with headroom

static BLEServer *bleServer = NULL;
static BLECharacteristic *imuSensorValueChar;

static unsigned long nextTxTimeMs = 0;

static auto bno = BNO055_Dummy();

class MyServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    connectionCount++;
    prevDeviceConnected = true;
  };

  void onDisconnect(BLEServer *server) {
    Serial.println("BLE disconnected");
    connectionCount--;
  }

  void tick() {
    if (connectionCount == 0 && prevDeviceConnected) {
      delay(500);
      Serial.println("Restart BLE advertising");
      bleServer->startAdvertising();
    }
  }

  int connectionCount = 0;
  bool prevDeviceConnected = false;
};

static MyServerCallbacks *myBLEServer = NULL;

void setup() {
  // Serial.begin(115200);
  Serial.begin(9600);

  BLEDevice::init(BLE_ADV_NAME);
  bleServer = BLEDevice::createServer();
  myBLEServer = new MyServerCallbacks();
  bleServer->setCallbacks(myBLEServer);

  addUARTService(bleServer);

  // IMU service and characteristics
  BLEService *imuService = bleServer->createService(BLE_IMU_SERVICE_UUID);
  imuSensorValueChar = imuService->createCharacteristic(
      BLE_IMU_SENSOR_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  imuSensorValueChar->addDescriptor(new BLE2902());
  auto *imuDeviceInfoChar = imuService->createCharacteristic(
      BLE_IMU_DEVICE_INFO_CHAR_UUID, BLECharacteristic::PROPERTY_READ);
  imuDeviceInfoChar->addDescriptor(new BLE2902());

  byte macAddress[6];
  WiFi.macAddress(macAddress);
  uint8_t macString[2 * sizeof macAddress];
  BLEUtils().buildHexData(macString, macAddress, 6);
  imuDeviceInfoChar->setValue(macString, sizeof macString);

  Serial.println("Starting BLE...");
  startUARTService();
  imuService->start();

  BLEAdvertising *adv = bleServer->getAdvertising();
  adv->addServiceUUID(BLE_IMU_SERVICE_UUID);
  adv->start();
}

void loop() {
  if (myBLEServer->connectionCount > 0) {
    tickUARTService();
  }
  unsigned long now = millis();
  // Doesn't account for time wraparound
  if (myBLEServer->connectionCount > 0 && now > nextTxTimeMs) {
    auto q = bno.getQuat();
    BLE_IMUMessage value(now);
    value.setQuaternion(q.w(), q.x(), q.y(), q.z());

    std::vector<uint8_t> payload = value.getPayload();
    imuSensorValueChar->setValue(payload.data(), payload.size());
    imuSensorValueChar->notify();

    nextTxTimeMs = now + TX_DELAY;
  }
  myBLEServer->tick();
}
