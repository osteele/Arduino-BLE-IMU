#pragma once
#ifndef BLESERVICEMANAGER_H
#define BLESERVICEMANAGER_H
#include <BLEDevice.h>
#include <BLEServer.h>
#include <HardwareSerial.h>
#include <algorithm>
#include "BLEServiceHandler.h"

class BLEServiceManager : public BLEServerCallbacks {
 public:
  BLEServer &bleServer;

  BLEServiceManager() : bleServer(*BLEDevice::createServer()) {
    bleServer.setCallbacks(this);
    adv_ = bleServer.getAdvertising();
  }

  uint32_t getConnectedCount() { return bleServer.getConnectedCount(); }

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
    uint32_t connectionCount = bleServer.getConnectedCount();
    if (connectionCount > 0) {
      std::for_each(serviceHandlers_.begin(), serviceHandlers_.end(),
                    std::mem_fun(&BLEServiceHandler::tick));
    }
    if (connectionCount == 0 && hasBeenConnected_) {
      delay(500);
      Serial.println("Restart BLE advertising");
      bleServer.startAdvertising();
    }
  }

 private:
  std::vector<BLEServiceHandler *> serviceHandlers_;
  BLEAdvertising *adv_;
  bool hasBeenConnected_ = false;

  void onConnect(BLEServer *server) {
    Serial.println("BLE connected");
    hasBeenConnected_ = true;
  };

  void onDisconnect(BLEServer *server) { Serial.println("BLE disconnected"); }
};
#endif /* BLESERVICEMANAGER_H */
