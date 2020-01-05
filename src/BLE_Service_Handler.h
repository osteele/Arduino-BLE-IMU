#pragma once
#include <BLEServer.h>
#include <BLEService.h>

class BLEServiceHandler {
 public:
  BLEServiceHandler(BLEServer *bleServer, const char *uuid)
      : bleService_(bleServer->createService(uuid)) {}

  virtual void start() { bleService_->start(); }

  virtual void tick() {}

 protected:
  BLEService *bleService_;
};
