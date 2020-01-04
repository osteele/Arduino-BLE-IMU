#include <cmath>
#include "utils.h"

class Quaternion {
 public:
  Quaternion(double w, double x, double y, double z)
      : _w(w), _x(x), _y(y), _z(z) {}
  const double& w() const { return _w; }
  const double& x() const { return _x; }
  const double& y() const { return _y; }
  const double& z() const { return _z; }

 private:
  double _w, _x, _y, _z;
};

class BNO055_Dummy {
 public:
  bool begin() { return true; }
  void setExtCrystalUse(bool) {}

  Quaternion getQuat() {
    unsigned long now = millis();
    static const float pi = std::acos(-1);
    const float s = now / 1000.0;
    const float euler[] = {static_cast<float>(pi / 10 * cos(1.2 * s)),
                           static_cast<float>(pi / 10 * cos(1.4 * s)),
                           static_cast<float>(fmod(s, 2 * pi))};
    float quat[4];
    euler2quat(euler, quat);
    return Quaternion(
        static_cast<double>(quat[0]), static_cast<double>(quat[1]),
        static_cast<double>(quat[2]), static_cast<double>(quat[3]));
  }
};
