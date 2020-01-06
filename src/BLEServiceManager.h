#pragma once
#include <BLEDevice.h>
#include <BLEServer.h>
#include <HardwareSerial.h>
#include <algorithm>
#include "BLE_Service_Handler.h"

class BLEServiceManager : public BLEServerCallbacks {
 public:
  BLEServer *bleServer;

  BLEServiceManager() : bleServer(BLEDevice::createServer()) {
    bleServer->setCallbacks(this);
    adv_ = bleServer->getAdvertising();
  }

  void addServiceHandler(BLEServiceHandler *handler, bool advertised = false) {
    serviceHandlers_.push_back(handler);
    if (advertised) {
      adv_->addServiceUUID(handler->uuid.c_str());
    }
  }

  void start() {
    std::for_each(serviceHandlers_.begin(), serviceHandlers_.end(),
                  std::mem_fun(&BLEServiceHandler::start));
    adv_->start();
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
  BLEAdvertising *adv_;
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
