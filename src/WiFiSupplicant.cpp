#include <SPIFFS.h>
#include <WiFi.h>
#include <map>

#include "WiFiSupplicant.h"

static const char WPA_SUPPLICANT_FNAME[] = "/wpa_supplicant.txt";

/** (ssid, password), ordered by preference. */
typedef std::vector<std::pair<const std::string, const std::string>> WiFiConfig;

/** ssid -> auth_mode */
typedef std::map<const std::string, wifi_auth_mode_t> WiFiScanMap;

static const WiFiConfig readSupplicants() {
  WiFiConfig ssidPasswords;

  if (!SPIFFS.begin(true)) {
    Serial.println("Unable to mount SPIFFS");
    return ssidPasswords;
  }
  if (!SPIFFS.exists(WPA_SUPPLICANT_FNAME)) {
    Serial.println("WPA supplicant file: does not exist");
    return ssidPasswords;
  }
  File file = SPIFFS.open(WPA_SUPPLICANT_FNAME);
  if (!file) {
    Serial.println("WPA supplicant file: Unable to open");
    return ssidPasswords;
  }

  std::string line, ssid;
  while (file.available()) {
    int c = file.read();
    if (c == '\n') {
      if (ssid.empty()) {
        ssid = line;
      } else {
        ssidPasswords.push_back(
            std::pair<std::string, std::string>(ssid, line));
        ssid.clear();
      }
      line.clear();
    } else
      line.append(1, c);
  }
  return ssidPasswords;
}

static const WiFiScanMap scanWiFi() {
  WiFiScanMap ssids;
  int16_t n = WiFi.scanNetworks();
  for (int16_t i = 0; i < n; ++i) {
    std::string ssid(WiFi.SSID(i).c_str());
    ssids[ssid] = WiFi.encryptionType(i);  // WIFI_AUTH_OPEN
  }
  return ssids;
}

bool WiFiSupplicant::connect() {
  const WiFiConfig supplicantSsids = readSupplicants();
  const WiFiScanMap localSsids = scanWiFi();
  auto item = std::find_if(
      supplicantSsids.begin(), supplicantSsids.end(),
      [&localSsids](const std::pair<std::string, std::string>& item) {
        const std::string& ssid = item.first;
        auto entry = localSsids.find(ssid);
        return entry != localSsids.end();
      });
  if (item == supplicantSsids.end()) {
    Serial.print("No known WiFi network found in ");
    int count = 0;
    for (const auto& item : localSsids) {
      if (++count > 1) Serial.print(", ");
      Serial.print(item.first.c_str());
    }
    Serial.print("\nSupplicant networks are ");
    for (const auto& item : supplicantSsids) {
      if (&item != &supplicantSsids.front()) Serial.print(", ");
      Serial.print(item.first.c_str());
    }
    Serial.println();
    return false;
  }

  auto ssid = item->first;
  auto password = item->second;

  Serial.printf("Connecting to WiFi %s...", ssid.c_str());

  WiFi.begin(ssid.c_str(), password.c_str());
  wl_status_t wifi_status;
  const int WIFI_CONTINUE_MASK = (1 << WL_IDLE_STATUS) | (1 << WL_DISCONNECTED);
  while ((1 << (wifi_status = WiFi.status())) & WIFI_CONTINUE_MASK) {
    Serial.print(".");
    delay(500);
  }
  if (wifi_status != WL_CONNECTED) {
    Serial.printf("failed with status=%d\n", wifi_status);
    return false;
  }
  Serial.println("success");
  Serial.printf("MAC address = %s\n", WiFi.macAddress().c_str());
  Serial.print("IP address = ");
  Serial.println(WiFi.localIP());
  return true;
}
