#include "utils.h"
#include <math.h>

std::string getMACAddress() {
  byte macAddress[6];
  WiFi.macAddress(macAddress);
  uint8_t macString[2 * sizeof macAddress + 1];
  BLEUtils().buildHexData(macString, macAddress, sizeof macAddress);
  return std::string((const char*)macString);
}

void euler2quat(const float euler[], float q[]) {
  float yaw = euler[0], pitch = euler[1], roll = euler[2];
  float c1 = cos(yaw / 2), s1 = sin(yaw / 2), c2 = cos(pitch / 2),
        s2 = sin(pitch / 2), c3 = cos(roll / 2), s3 = sin(roll / 2);
  float w = c1 * c2 * c3 - s1 * s2 * s3, x = s1 * s2 * c3 + c1 * c2 * s3,
        y = s1 * c2 * c3 + c1 * s2 * s3, z = c1 * s2 * c3 - s1 * c2 * s3;
  q[0] = x;
  q[1] = y;
  q[2] = z;
  q[3] = w;
}
