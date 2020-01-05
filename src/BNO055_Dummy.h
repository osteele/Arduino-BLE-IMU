#pragma once
#include <Adafruit_BNO055.h>
#include <algorithm>
#include <cmath>
#include "utils.h"

class BNO055Base {
 public:
  virtual bool begin() = 0;
  // virtual void setExtCrystalUse(bool);
  virtual void getCalibration(uint8_t* system, uint8_t* gyro, uint8_t* accel,
                              uint8_t* mag) = 0;
  virtual imu::Quaternion getQuat() = 0;
};

template <class T>
class BNO055Adaptor : public BNO055Base {
 public:
  BNO055Adaptor(T& bno) : _base(bno) {}
  bool begin() { return _base.begin(); }
  // virtual setExtCrystalUse(bool flag) : {}
  void getCalibration(uint8_t* system, uint8_t* gyro, uint8_t* accel,
                      uint8_t* mag) {
    _base.getCalibration(system, gyro, accel, mag);
  };

  imu::Quaternion getQuat() { return _base.getQuat(); }

 private:
  T _base;
};

class BNO055Dummy : public BNO055Base {
 public:
  BNO055Dummy() : createdAt_(millis()) {}
  bool begin() { return true; }
  void setExtCrystalUse(bool) {}

  void getCalibration(uint8_t* system, uint8_t* gyro, uint8_t* accel,
                      uint8_t* mag) {
    uint8_t c = std::min(3, static_cast<int>((millis() - createdAt_) / 1000));
    *system = c;
    *gyro = c;
    *accel = c;
    *mag = c;
  }

  imu::Quaternion getQuat() {
    unsigned long now = millis();
    static const float pi = std::acos(-1);
    const float s = now / 1000.0;
    const float euler[] = {static_cast<float>(pi / 10 * cos(1.2 * s)),
                           static_cast<float>(pi / 10 * cos(1.4 * s)),
                           static_cast<float>(fmod(s, 2 * pi))};
    float quat[4];
    euler2quat(euler, quat);
    return imu::Quaternion(
        static_cast<double>(quat[0]), static_cast<double>(quat[1]),
        static_cast<double>(quat[2]), static_cast<double>(quat[3]));
  }

 private:
  unsigned long createdAt_;
};
