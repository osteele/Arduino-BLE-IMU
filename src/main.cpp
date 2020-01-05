#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>
#include "BLE_IMU_Service.h"
#include "BLE_MAC_Address_Service.h"
#include "BLE_UART_Service.h"

static const char BLE_ADV_NAME[] = "ESP32 IMU";

static BLEServer *bleServer = NULL;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    connectionCount++;
    prevDeviceConnected = true;
  };

  void onDisconnect(BLEServer *server) {
    Serial.println("BLE disconnected");
    connectionCount--;
  }

 public:
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

static MyServerCallbacks *myBLEServer;
static UARTServiceHandler *uartServiceHandler;
static IMUServiceHandler *imuServiceHandler;
static BLEMACAddressServiceHandler *macAddressServiceHandler;

void setup() {
  // Serial.begin(115200);
  Serial.begin(9600);

  BLEDevice::init(BLE_ADV_NAME);
  bleServer = BLEDevice::createServer();
  myBLEServer = new MyServerCallbacks();
  bleServer->setCallbacks(myBLEServer);
  uartServiceHandler = new UARTServiceHandler(bleServer);
  imuServiceHandler = new IMUServiceHandler(bleServer);
  macAddressServiceHandler = new BLEMACAddressServiceHandler(bleServer);

  Serial.println("Starting BLE...");
  uartServiceHandler->start();
  imuServiceHandler->start();
  macAddressServiceHandler->start();

  BLEAdvertising *adv = bleServer->getAdvertising();
  adv->addServiceUUID(BLE_IMU_SERVICE_UUID);
  adv->start();
}

void loop() {
  if (myBLEServer->connectionCount > 0) {
    uartServiceHandler->tick();
    imuServiceHandler->tick();
  }
  myBLEServer->tick();
}
