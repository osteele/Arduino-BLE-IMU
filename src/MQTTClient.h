#pragma once
#ifndef _MQTTCLIENT_H_
#define _MQTTCLIENT_H_
#include <PubSubClient.h>
#include <WiFi.h>
#include <string>

class MQTTClient {
 public:
  MQTTClient() : pubSubClient_(wifiClient_) {}
  bool connect();
  bool connected() { return wifiClient_.connected(); }
  bool publish(const char payload[]);
  bool publish(std::vector<uint8_t>& v) {
    std::string str(v.begin(), v.end());
    return publish(str.c_str());
  }

 private:
  WiFiClient wifiClient_;
  PubSubClient pubSubClient_;
  std::string topic_;
};

#endif /* _MQTTCLIENT_H_ */
