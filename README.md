# Arduino BLE IMU

Configures an Arduino to publish BNO055 orientation data over a Bluetooth Low
Energy (BLE) connection, for use with
[osteele/imu-tools](https://github.com/osteele/imu-tools).

`src/bt_imu_service.h` defines the service and characteristic IDs.

Characteristic data are transmitted in little-endian order, to match the
standard GATT profile characteristics such as the heart-rate characteristic.

This is a work in progress.
