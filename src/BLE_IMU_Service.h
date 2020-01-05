#include <stdint.h>
#include <string.h>
#include <cassert>
#include <vector>
#include "BNO055_Dummy.h"

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
  BLE_IMU_LINEAR_FLAG = 0x40,
  BLE_IMU_GRAVITY_FLAG = 0x80
};

class BLE_IMUMessage {
 public:
  BLE_IMUMessage(unsigned long timestamp) : timestamp_(timestamp){};

  void setQuaternion(float quat[4]) {
    memcpy(quat_, quat, sizeof quat_);
    flags_ |= BLE_IMU_QUATERNION_FLAG;
  }

  void setQuaternion(double quat[4]) {
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
    uint8_t data[64] = {
        BLE_IMU_MESSAGE_VERSION,
        flags_,
        static_cast<uint8_t>(timestamp_),
        static_cast<uint8_t>(timestamp_ >> 8),
    };
    uint8_t *p = &data[4];
    if (flags_ & BLE_IMU_QUATERNION_FLAG) {
      assert(p - data + sizeof quat_ <= sizeof data);
      memcpy(p, quat_, sizeof quat_);
      p += sizeof quat_;
    }
    std::vector<uint8_t> vec(data, p);
    return vec;
  }

 private:
  uint8_t flags_ = 0;
  unsigned long timestamp_;
  float quat_[4];
};

static const int TX_DELAY = (1000 - 10) / 60;  // 60 fps, with headroom

class IMUServiceHandler {
 public:
  IMUServiceHandler(BLEServer *bleServer) {
    bleService_ = bleServer->createService(BLE_IMU_SERVICE_UUID);

    imuSensorValueChar_ = bleService_->createCharacteristic(
        BLE_IMU_SENSOR_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    imuSensorValueChar_->addDescriptor(new BLE2902());

    imuCalibrationChar_ = bleService_->createCharacteristic(
        BLE_IMU_CALIBRATION_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    imuCalibrationChar_->addDescriptor(new BLE2902());
  }

  void start() {
    bleService_->start();
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
      auto q = bno_.getQuat();
      BLE_IMUMessage value(now);
      value.setQuaternion(q.w(), q.x(), q.y(), q.z());

      std::vector<uint8_t> payload = value.getPayload();
      imuSensorValueChar_->setValue(payload.data(), payload.size());
      imuSensorValueChar_->notify();

      nextTxTimeMs_ = now + TX_DELAY;
    }
  }

 private:
  BLEService *bleService_;
  BLECharacteristic *imuSensorValueChar_;
  BLECharacteristic *imuCalibrationChar_;
  BNO055_Dummy bno_;
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
