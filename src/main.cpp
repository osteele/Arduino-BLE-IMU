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
static BNO055Base* imuSensor;

#ifdef FIREBEETLE
#define SPICLK 22
#define SPIDAT 21
#else
#define SPICLK 22
#define SPIDAT 23
#endif

static BNO055Base* getIMU() {
  Serial.printf("SPI = {data: %d, clock: %d}\n", SPIDAT, SPICLK);
  Wire.begin(SPIDAT, SPICLK);

  if (bno055.begin()) {
    Serial.println("Connected to BNO055");
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
  imuSensor = getIMU();

  BLEDevice::init(bleDeviceName.c_str());
  bleServiceManager = new BLEServiceManager();
  new BLEIMUServiceHandler(*bleServiceManager, *imuSensor);
  new BLEMACAddressServiceHandler(*bleServiceManager);
  new BLEUARTServiceHandler(*bleServiceManager);

  Serial.printf("Starting BLE (device name=%s)\n", bleDeviceName.c_str());
  bleServiceManager->start();
}

static const int MQTT_IMU_TX_FREQ = 60;
static const int MQTT_TX_DELAY = (1000 - 10) / MQTT_IMU_TX_FREQ;
static unsigned long nextTxTimeMs = 0;

void loop() {
  bleServiceManager->tick();

  unsigned long now = millis();
  if (bleServiceManager->getConnectedCount() == 0 && mqttClient.connected() &&
      now > nextTxTimeMs) {
    BLE_IMUMessage message(now);
    auto q = imuSensor->getQuat();
    message.setQuaternion(q.w(), q.x(), q.y(), q.z());
    message.setAccelerometer(
        imuSensor->getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER));
    message.setGyroscope(
        imuSensor->getVector(Adafruit_BNO055::VECTOR_GYROSCOPE));
    message.setMagnetometer(
        imuSensor->getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER));
    message.setLinearAcceleration(
        imuSensor->getVector(Adafruit_BNO055::VECTOR_LINEARACCEL));

    std::vector<uint8_t> payload = message.getPayload();
    if (!mqttClient.publish(payload)) {
      Serial.println("MQTT publish: failed");
    }
    nextTxTimeMs = now + MQTT_TX_DELAY;
  }
}
