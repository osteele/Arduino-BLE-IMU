#pragma once
#ifndef _BLEIMUSERVICE_H
#define _BLEIMUSERVICE_H
#include <Adafruit_BNO055.h>
#include <stdint.h>

#include <cassert>
#include <vector>

#include "BLEServiceHandler.h"
#include "BNO055Dummy.h"

const char BLE_IMU_SERVICE_UUID[] = "509B0001-EBE1-4AA5-BC51-11004B78D5CB";
const char BLE_IMU_SENSOR_CHAR_UUID[] = "509B0002-EBE1-4AA5-BC51-11004B78D5CB";
const char BLE_IMU_CALIBRATION_CHAR_UUID[] =
    "509B0003-EBE1-4AA5-BC51-11004B78D5CB";

const uint8_t BLE_IMU_MESSAGE_VERSION = 1;

enum BLE_IMUFieldBits {
  BLE_IMU_ACCEL_FLAG = 0x01,
  BLE_IMU_MAG_FLAG = 0x02,
  BLE_IMU_GYRO_FLAG = 0x04,
  BLE_IMU_CALIBRATION_FLAG = 0x08,
  BLE_IMU_EULER_FLAG = 0x10,
  BLE_IMU_QUATERNION_FLAG = 0x20,
  BLE_IMU_LINEAR_ACCEL_FLAG = 0x40,
  BLE_IMU_GRAVITY_FLAG = 0x80
};

class BLE_IMUMessage {
 public:
  BLE_IMUMessage(unsigned long timestamp) : timestamp_(timestamp){};

  void setAccelerometer(const imu::Vector<3> &vec) {
    accel_ = vec;
    flags_ |= BLE_IMU_ACCEL_FLAG;
  }

  void setGyroscope(imu::Vector<3> vec) {
    gyro_ = vec;
    flags_ |= BLE_IMU_GYRO_FLAG;
  }

  void setMagnetometer(const imu::Vector<3> &vec) {
    mag_ = vec;
    flags_ |= BLE_IMU_MAG_FLAG;
  }

  void setLinearAcceleration(const imu::Vector<3> &vec) {
    linear_ = vec;
    flags_ |= BLE_IMU_LINEAR_ACCEL_FLAG;
  }

  void setQuaternion(const float quat[4]) {
    memcpy(quat_, quat, sizeof quat_);
    flags_ |= BLE_IMU_QUATERNION_FLAG;
  }

  void setQuaternion(const double quat[4]) {
    float q[4] = {
        static_cast<float>(quat[0]),
        static_cast<float>(quat[1]),
        static_cast<float>(quat[2]),
        static_cast<float>(quat[3]),
    };
    setQuaternion(q);
  }

  void setQuaternion(double w, double x, double y, double z) {
    double q[] = {w, x, y, z};
    setQuaternion(q);
  }

  std::vector<uint8_t> getPayload() {
    uint8_t buf[240] = {
        BLE_IMU_MESSAGE_VERSION,
        flags_,
        static_cast<uint8_t>(timestamp_),
        static_cast<uint8_t>(timestamp_ >> 8),
    };
    uint8_t *p = &buf[4];
    if (flags_ & BLE_IMU_QUATERNION_FLAG) {
      assert(p - buf + sizeof quat_ <= sizeof buf);
      memcpy(p, quat_, sizeof quat_);
      p += sizeof quat_;
    }
    if (flags_ & BLE_IMU_ACCEL_FLAG) {
      p = this->appendVector_(buf, sizeof buf, p, accel_);
    }
    if (flags_ & BLE_IMU_GYRO_FLAG) {
      p = this->appendVector_(buf, sizeof buf, p, gyro_);
    }
    if (flags_ & BLE_IMU_MAG_FLAG) {
      p = this->appendVector_(buf, sizeof buf, p, mag_);
    }
    if (flags_ & BLE_IMU_LINEAR_ACCEL_FLAG) {
      p = this->appendVector_(buf, sizeof buf, p, linear_);
    }
    std::vector<uint8_t> vec(buf, p);
    return vec;
  }

 private:
  uint8_t flags_ = 0;
  unsigned long timestamp_;
  float quat_[4];
  imu::Vector<3> accel_, gyro_, mag_, linear_;

  uint8_t *appendVector_(uint8_t *buf, size_t size, uint8_t *p,
                         const imu::Vector<3> &vec) {
    float v[3] = {
        static_cast<float>(vec.x()),
        static_cast<float>(vec.y()),
        static_cast<float>(vec.z()),
    };
    assert(p - buf + sizeof v <= size);
    memcpy(p, v, sizeof v);
    p += sizeof v;
    return p;
  };
};

// 60 fps, with headroom
#define BLE_IMU_TX_FREQ 60
static const int BLE_IMU_TX_DELAY = (1000 - 10) / BLE_IMU_TX_FREQ;

class BLEIMUServiceHandler : public BLEServiceHandler {
 public:
  const bool INCLUDE_ALL_VALUES = true;

  BLEIMUServiceHandler(BLEServiceManager &manager, BNO055Base &sensor)
      : BLEServiceHandler(manager, BLE_IMU_SERVICE_UUID), bno_(sensor) {
    imuSensorValueChar_ = bleService_->createCharacteristic(
        BLE_IMU_SENSOR_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    imuSensorValueChar_->addDescriptor(new BLE2902());

    imuCalibrationChar_ = bleService_->createCharacteristic(
        BLE_IMU_CALIBRATION_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    imuCalibrationChar_->addDescriptor(new BLE2902());
  }

  void start() {
    BLEServiceHandler::start();
    setCalibrationValue();
  }

  void tick() {
    unsigned long now = millis();
    // Doesn't account for time wraparound
    if (now > nextTxTimeMs_) {
      std::array<uint8_t, 4> calibration;
      bno_.getCalibration(&calibration[0], &calibration[1], &calibration[2],
                          &calibration[3]);
      if (calibration != calibration_) {
        setCalibrationValue();
        imuCalibrationChar_->notify();
      }

      BLE_IMUMessage message(now);
      auto quat = bno_.getQuat();
      message.setQuaternion(quat.w(), quat.x(), quat.y(), quat.z());
      if (INCLUDE_ALL_VALUES) {
        message.setAccelerometer(
            bno_.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER));
        message.setGyroscope(bno_.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE));
        message.setMagnetometer(
            bno_.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER));
        message.setLinearAcceleration(
            bno_.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL));
      }

      std::vector<uint8_t> payload = message.getPayload();
      imuSensorValueChar_->setValue(payload.data(), payload.size());
      imuSensorValueChar_->notify();

      nextTxTimeMs_ = now + BLE_IMU_TX_DELAY;
    }
  }

 private:
  BLECharacteristic *imuSensorValueChar_;
  BLECharacteristic *imuCalibrationChar_;
  BNO055Base &bno_;
  std::array<uint8_t, 4> calibration_ = {{0, 0, 0, 0}};
  unsigned long nextTxTimeMs_ = 0;

  void setCalibrationValue() {
    std::array<uint8_t, 4> calibration;
    bno_.getCalibration(&calibration[0], &calibration[1], &calibration[2],
                        &calibration[3]);
    imuCalibrationChar_->setValue(calibration.data(), calibration.size());
    calibration_ = calibration;
  }
};

#endif /* _BLEIMUSERVICE_H */
