#pragma once
#include <BLEServer.h>
#include <BLEService.h>

class BLEServiceHandler {
 public:
  std::string uuid;
  BLEServiceHandler(BLEServer *bleServer, const char *uuid)
      : uuid(uuid), bleService_(bleServer->createService(uuid)) {}

  virtual void start() { bleService_->start(); }

  virtual void tick() {}

 protected:
  BLEService *bleService_;
};
