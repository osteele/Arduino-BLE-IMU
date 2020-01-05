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

class BLEServiceManager : public BLEServerCallbacks {
 public:
  BLEServer *bleServer;

  BLEServiceManager() : bleServer(BLEDevice::createServer()) {
    bleServer->setCallbacks(this);
  }

  void addServiceHandler(BLEServiceHandler *handler) {
    serviceHandlers_.push_back(handler);
  }

  void start() {
    std::for_each(serviceHandlers_.begin(), serviceHandlers_.end(),
                  std::mem_fun(&BLEServiceHandler::start));
  }

  void advertise() {
    BLEAdvertising *adv = bleServer->getAdvertising();
    adv->addServiceUUID(BLE_IMU_SERVICE_UUID);
    adv->start();
  }

  void tick() {
    if (connectionCount_ > 0) {
      std::for_each(serviceHandlers_.begin(), serviceHandlers_.end(),
                    std::mem_fun(&BLEServiceHandler::tick));
    }
    if (connectionCount_ == 0 && prevDeviceConnected_) {
      delay(500);
      Serial.println("Restart BLE advertising");
      bleServer->startAdvertising();
    }
  }

 private:
  std::vector<BLEServiceHandler *> serviceHandlers_;
  int connectionCount_ = 0;
  bool prevDeviceConnected_ = false;

  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    connectionCount_++;
    prevDeviceConnected_ = true;
  };

  void onDisconnect(BLEServer *server) {
    Serial.println("BLE disconnected");
    connectionCount_--;
  }
};

static BLEServiceManager *bleServiceManager;

void setup() {
  // Serial.begin(115200);
  Serial.begin(9600);

  BLEDevice::init(BLE_ADV_NAME);
  bleServiceManager = new BLEServiceManager();
  bleServiceManager->addServiceHandler(
      new BLE_IMUServiceHandler(bleServiceManager->bleServer));
  bleServiceManager->addServiceHandler(
      new BLE_MACAddressServiceHandler(bleServiceManager->bleServer));
  bleServiceManager->addServiceHandler(
      new BLE_UARTServiceHandler(bleServiceManager->bleServer));

  Serial.println("Starting BLE...");
  bleServiceManager->start();
  bleServiceManager->advertise();
}

void loop() { bleServiceManager->tick(); }
