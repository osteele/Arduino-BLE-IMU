#pragma once
#include <SPIFFS.h>

class Config {
 public:
  static Config& getInstance() {
    static Config instance;
    return instance;
  }
  Config(Config const&) = delete;
  void operator=(Config const&) = delete;

  std::string getBLEName(const std::string defaultValue) {
    if (!SPIFFS.begin(true)) {
      Serial.println("Unable to mount SPIFFS");
      return defaultValue;
    }
    File file = SPIFFS.open(BLE_DEVICE_NAME_FILE);
    if (!file) return defaultValue;

    std::string value;
    while (file.available()) {
      value.append(1, file.read());
    }
    file.close();
    Serial.print("Read saved BLE device name: ");
    Serial.println(value.c_str());
    return value;
  }

  void setBLEName(const std::string value) {
    File file = SPIFFS.open(BLE_DEVICE_NAME_FILE, FILE_WRITE);
    if (file) {
      file.print(value.c_str());
      file.close();
      Serial.print("Set BLE device name: ");
      Serial.println(value.c_str());
    }
  }

 private:
  Config() {}

  static constexpr const char* BLE_DEVICE_NAME_FILE = "/ble-name.txt";
};
