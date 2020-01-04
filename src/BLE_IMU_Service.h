const char BLE_IMU_SERVICE_UUID[] = "509B8001-EBE1-4AA5-BC51-11004B78D5CB";
const char BLE_IMU_SENSOR_CHAR_UUID[] = "509B8002-EBE1-4AA5-BC51-11004B78D5CB";
const char BLE_IMU_DEVICE_INFO_CHAR_UUID[] = "509B8003-EBE1-4AA5-BC51-11004B78D5CB";

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
  BLE_IMUMessage(unsigned long timestamp) : m_timestamp(timestamp){};

  void setQuaternion(float quat[4]) {
    memcpy(m_quat, quat, sizeof m_quat);
    m_flags |= BLE_IMU_QUATERNION_FLAG;
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
        m_flags,
        static_cast<uint8_t>(m_timestamp),
        static_cast<uint8_t>(m_timestamp >> 8),
    };
    uint8_t *p = &data[4];
    if (m_flags & BLE_IMU_QUATERNION_FLAG) {
      assert(p - data + sizeof m_quat <= sizeof data);
      memcpy(p, m_quat, sizeof m_quat);
      p += sizeof m_quat;
    }
    std::vector<uint8_t> vec(data, p);
    return vec;
  }

 private:
  uint8_t m_flags = 0;
  unsigned long m_timestamp;
  float m_quat[4];
};
