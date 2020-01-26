#include "MQTTClient.h"
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <string>

static const char MQTT_CONFIG_FNAME[] = "/mqtt.config";

static std::string mqttServer;
static int mqttPort = 18623;
static std::string mqttUser;
static std::string mqttPassword;

WiFiClient wifiClient;
static PubSubClient client(wifiClient);

static std::string topic;

static void readConfig();

void mqttConnect() {
  readConfig();

  client.setServer(mqttServer.c_str(), mqttPort);
  Serial.printf("Connecting to mqtt://%s@%s:%d...", mqttUser.c_str(),
                mqttServer.c_str(), mqttPort);
  if (client.connect("ESP32Client", mqttUser.c_str(), mqttPassword.c_str())) {
    Serial.println("connected");
  } else {
    Serial.printf("failed with state %d\n", client.state());
    return;
  }

  std::string device_id(WiFi.macAddress().c_str());
  device_id.erase(std::remove(device_id.begin(), device_id.end(), ':'),
                  device_id.end());
  std::transform(device_id.begin(), device_id.end(), device_id.begin(),
                 ::tolower);
  Serial.printf("device_id = %s\n", device_id.c_str());

  topic = "/imu/" + device_id;
  mqttSend();
}

static void readConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Unable to mount SPIFFS");
    return;
  }
  if (!SPIFFS.exists(MQTT_CONFIG_FNAME)) {
    Serial.println("MQTT config file: does not exist");
    return;
  }
  File file = SPIFFS.open(MQTT_CONFIG_FNAME);
  if (!file) {
    Serial.println("MQTT config file: Unable to open");
    return;
  }

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

  if (fields.size() < 4) {
    Serial.printf("MQTT config file: %d fields found; 4 expected\n",
                  fields.size());
    return;
  }

  mqttServer = fields[0];
  mqttPort = atoi(fields[1].c_str());
  mqttUser = fields[2];
  mqttPassword = fields[3];
}

void mqttSend() { client.publish(topic.c_str(), "hello world"); }
