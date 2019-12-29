#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>

const char BLE_ADV_NAME[] = "NYUSHIMA";
const char NF_UART_SERVICE_UUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char NF_UART_RX_CHAR_UUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char NF_UART_TX_CHAR_UUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BLEServer *bleServer = NULL;
BLECharacteristic *txChar;
BLECharacteristic *rxChar;
bool deviceConnected = false;
bool prevDeviceConnected = false;
unsigned long nextTxTimeMs = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *server) {
    Serial.println("BLE disconnected");
    deviceConnected = false;
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
  BLEService *bleService = bleServer->createService(NF_UART_SERVICE_UUID);

  txChar = bleService->createCharacteristic(NF_UART_TX_CHAR_UUID,
                                            BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());

  rxChar = bleService->createCharacteristic(NF_UART_RX_CHAR_UUID,
                                            BLECharacteristic::PROPERTY_WRITE);
  rxChar->setCallbacks(new UARTRxCallbacks());

  Serial.println("Starting BLE...");
  bleService->start();
  bleServer->getAdvertising()->start();
}

void loop() {
  if (deviceConnected && millis() > nextTxTimeMs) {
    static char buffer[10];
    int len = snprintf(buffer, sizeof buffer, "%ld\n", millis());
    if (0 <= len && len <= sizeof buffer) {
      txChar->setValue((uint8_t *)buffer, len);
      txChar->notify();
    } else {
      Serial.println("Tx buffer overflow");
    }
    nextTxTimeMs = millis() + 1000;
  }

  if (!deviceConnected && prevDeviceConnected) {
    delay(500);
    Serial.println("Restart BLE advertising");
    bleServer->startAdvertising();
  }
  prevDeviceConnected = deviceConnected;
}
