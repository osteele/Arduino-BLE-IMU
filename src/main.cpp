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
static BNO055Base* bno;

#ifndef FIREBEETLE
#define SPICLK 22
#define SPIDAT 23
#else
#define SPICLK 22
#define SPIDAT 21
#endif

static BNO055Base* getBNO055() {
  Serial.printf("SPI = %d %d\n", SPIDAT, SPICLK);
  Wire.begin(SPIDAT, SPICLK);

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
  new BLEIMUServiceHandler(bleServiceManager, bno);
  new BLEMACAddressServiceHandler(bleServiceManager);
  new BLEUARTServiceHandler(bleServiceManager);

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

    if (!mqttClient.publish(payload)) {
      Serial.println("MQTT publish: failed");
    }
    nextTxTimeMs = now + MQTT_TX_DELAY;
  }
}
