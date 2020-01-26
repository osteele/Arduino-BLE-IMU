#include <SPIFFS.h>

#include "MQTTClient.h"

static const char MQTT_CONFIG_FNAME[] = "/mqtt.config";

class MQTTConfig {
 public:
  std::string host;
  int port = 18623;
  std::string user;
  std::string password;

  bool read() {
    if (!SPIFFS.begin(true)) {
      Serial.println("Unable to mount SPIFFS");
      return false;
    }
    if (!SPIFFS.exists(MQTT_CONFIG_FNAME)) {
      Serial.println("MQTT config file: does not exist");
      return false;
    }
    File file = SPIFFS.open(MQTT_CONFIG_FNAME);
    if (!file) {
      Serial.println("MQTT config file: Unable to open");
      return false;
    }

    static std::vector<std::string> fields = readFields(file);

    if (fields.size() < 4) {
      Serial.printf("MQTT config file: %d fields found; 4 expected\n",
                    fields.size());
      return false;
    }

    host = fields[0];
    port = atoi(fields[1].c_str());
    user = fields[2];
    password = fields[3];
    return true;
  }

 private:
  std::vector<std::string> readFields(File file) {
    static std::vector<std::string> fields;
    std::string field;
    while (file.available()) {
      int c = file.read();
      if (c == '\n') {
        fields.push_back(field);
        field.clear();
      } else {
        field.append(1, c);
      }
    }
    if (!field.empty()) {
      fields.push_back(field);
    }
    return fields;
  }
};

bool MQTTClient::connect() {
  MQTTConfig config;
  if (!config.read()) return false;

  pubSubClient_.setServer(config.host.c_str(), config.port);
  Serial.printf("Connecting to mqtt://%s@%s:%d...", config.user.c_str(),
                config.host.c_str(), config.port);
  if (!pubSubClient_.connect("ESP32Client", config.user.c_str(),
                             config.password.c_str())) {
    Serial.printf("failed with state %d\n", pubSubClient_.state());
    return false;
  }
  Serial.println("connected");

  std::string device_id(WiFi.macAddress().c_str());
  device_id.erase(std::remove(device_id.begin(), device_id.end(), ':'),
                  device_id.end());
  std::transform(device_id.begin(), device_id.end(), device_id.begin(),
                 ::tolower);
  Serial.printf("device_id = %s\n", device_id.c_str());

  topic_ = "imu/" + device_id;
  return true;
}

bool MQTTClient::publish(const char payload[]) {
  bool status = pubSubClient_.publish(topic_.c_str(), payload);
  if (!status) {
    Serial.print("mqtt publish: ");
    Serial.println(status);
  }
  return status;
}
