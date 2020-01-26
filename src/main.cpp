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
#include "Config.h"
#include "MQTTClient.h"
#include "WiFiSupplicant.h"

static const char BLE_ADV_NAME[] = "ESP32 IMU";

static Adafruit_BNO055 bno055;
static BLEServiceManager* bleServiceManager;
static MQTTClient mqttClient;
static WiFiSupplicant wifiSupplicant;
BNO055Base* bno;

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

void setup() {
  Serial.begin(115200);

  if (wifiSupplicant.connect()) mqttClient.connect();

  std::string bleDeviceName =
      Config::getInstance().getBLEDeviceName(BLE_ADV_NAME);
  bno = getBNO055();

  BLEDevice::init(bleDeviceName.c_str());
  bleServiceManager = new BLEServiceManager();
  bleServiceManager->addServiceHandler(
      new BLE_IMUServiceHandler(&bleServiceManager->bleServer, *bno), true);
  bleServiceManager->addServiceHandler(
      new BLE_MACAddressServiceHandler(&bleServiceManager->bleServer));
  bleServiceManager->addServiceHandler(
      new BLE_UARTServiceHandler(&bleServiceManager->bleServer));

  Serial.print("Starting BLE (device name=");
  Serial.print(bleDeviceName.c_str());
  Serial.println(")");
  bleServiceManager->start();
}

static const int MQTT_TX_DELAY = (1000 - 10) / 60;  // 60 fps, with headroom
static unsigned long nextTxTimeMs = 0;

void loop() {
  bleServiceManager->tick();

  unsigned long now = millis();
  if (bleServiceManager->getConnectedCount() == 0 && mqttClient.connected() &&
      now > nextTxTimeMs) {
    BLE_IMUMessage value(now);
    auto q = bno->getQuat();
    value.setQuaternion(q.w(), q.x(), q.y(), q.z());
    std::vector<uint8_t> payload = value.getPayload();
    mqttClient.publish(payload);
    nextTxTimeMs = now + MQTT_TX_DELAY;
  }
}
