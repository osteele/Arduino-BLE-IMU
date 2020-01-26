#pragma once
#ifndef _MQTTCLIENT_H_
#define _MQTTCLIENT_H_
#include <PubSubClient.h>
#include <WiFi.h>
#include <string>

class MQTTClient {
 public:
  MQTTClient() : client_(wifiClient_) {}
  bool connect();
  void send();

 private:
  WiFiClient wifiClient_;
  PubSubClient client_;
  std::string topic_;
};

#endif /* _MQTTCLIENT_H_ */
