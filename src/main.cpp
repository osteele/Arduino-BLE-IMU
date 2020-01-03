#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>
#include "bt_imu_service.h"

const char BLE_ADV_NAME[] = "NYUSHIMA IMU";

const char NF_UART_SERVICE_UUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char NF_UART_RX_CHAR_UUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char NF_UART_TX_CHAR_UUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

const bool SEND_UART_TX_HEARTBEAT = false;
const int TX_DELAY = 900 / 60;  // 60 fps, with headroom

BLEServer *bleServer = NULL;
BLECharacteristic *txChar;
BLECharacteristic *rxChar;
BLECharacteristic *imuQuatChar;

int deviceConnectedCount = 0;
bool prevDeviceConnected = false;

unsigned long nextTxTimeMs = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    deviceConnectedCount++;
  };

  void onDisconnect(BLEServer *server) {
    Serial.println("BLE disconnected");
    deviceConnectedCount--;
  }
};

class UARTRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *ch) {
    std::string value = ch->getValue();
    if (value.length() > 0) {
      Serial.print("Rx: ");
      Serial.write((value + "\0").c_str());
      if (value[value.length() - 1] != '\n') {
        Serial.println();
      }
      if (value == "ping\n") {
        Serial.write("Tx: pong\n");
        static uint8_t data[] = "pong\n";
        txChar->setValue(data, sizeof data - 1);
        txChar->notify();
        delay(500);
      }
    }
  }
};

void setup() {
  // Serial.begin(115200);
  Serial.begin(9600);
  BLEDevice::init(BLE_ADV_NAME);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new MyServerCallbacks());

  // UART service and charadcteristics
  BLEService *uartService = bleServer->createService(NF_UART_SERVICE_UUID);
  txChar = uartService->createCharacteristic(
      NF_UART_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());
  rxChar = uartService->createCharacteristic(NF_UART_RX_CHAR_UUID,
                                             BLECharacteristic::PROPERTY_WRITE);
  rxChar->setCallbacks(new UARTRxCallbacks());

  // IMU service and charadcteristics
  BLEService *imuService = bleServer->createService(BT_IMU_SERVICE_UUID);
  imuQuatChar = imuService->createCharacteristic(
      BT_IMU_QUAT_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  imuQuatChar->addDescriptor(new BLE2902());

  Serial.println("Starting BLE...");
  uartService->start();
  imuService->start();
  bleServer->getAdvertising()->start();
}

void loop() {
  unsigned long now = millis();
  // This test assumes the device is rebooted in less than 49 days
  if (deviceConnectedCount > 0 && now > nextTxTimeMs) {
    if (SEND_UART_TX_HEARTBEAT) {
      static char buffer[10];
      int len = snprintf(buffer, sizeof buffer, "%ld\n", now);
      if (0 <= len && len <= sizeof buffer) {
        txChar->setValue((uint8_t *)buffer, len);
        txChar->notify();
      } else {
        Serial.println("Tx buffer overflow");
      }
    }

    // const float t = now / 100.0;
    const float quat[] = {1, 2, 1000, 2000000};
    imuQuatChar->setValue((uint8_t *)quat, sizeof quat);
    imuQuatChar->notify();

    nextTxTimeMs = now + TX_DELAY;
  }

  if (!deviceConnectedCount && prevDeviceConnected) {
    delay(500);
    Serial.println("Restart BLE advertising");
    bleServer->startAdvertising();
  }
  prevDeviceConnected = deviceConnectedCount > 0;
}
