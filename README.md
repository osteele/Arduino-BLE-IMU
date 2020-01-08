# Arduino BLE IMU

Configures an Arduino to publish BNO055 orientation data over a Bluetooth Low
Energy (BLE) connection, for use with
[osteele/imu-tools](https://github.com/osteele/imu-tools).

## BLE Services

The software defines these BLE services:

The **IMU Service** includes these characteristics:

* Sensor (READ): A packed structure of sensors readings. The message format is:
  * Version (uint8): 1
  * Flags (uint8): specifies which of the following fields are present.
  * Time (uint16): the low 16 bits of the MCU's millisecond time
  * Quaternion (optional; 4 x float32): quaternion w, x, y, z
  * [Other sensors are not currently implemented]

  This protocol has the property that a message with just the Quaternion
  position is exactly 20 bytes, or one BLE 4.0 packet.

* Calibration (READ, NOTIFY; 4 x uint_8): The system, gyro, accel, and mag
  calibration values, each ranging 0..3. These are NOTIFYed when they change
  (from initial assumed values of 0).

The Service and Characteristic UUIDs, and the constants used in the Flags byte,
are defined in `./src/BLEIMUService.h`.

Characteristic data are transmitted in little-endian order, not network order.
This matches the standard GATT profile characteristics such as the heart-rate
characteristic.

The device is polled 60 times / second. The sensor characteristic is always
written and notified. The polling interval is a constant that is defined in the
code. The system doesn't appear to be capable of transmitting at greater than
~126 samples/second. For greater rates, consider MQTT, or onboard processing of
the data.

The **MAC Address Service** publishes the device's Ethernet MAC address, and the
BLE device name. It defines these characteristics:

* MAC address (READ; string). imu-tools uses the MAC address as a device id that
  persists across connections (the Web BLE API doesn't make the BLE device id
  available to code). The MAC address is used instead of the BLE address so that
  a device that publishes both to WiFi and BLE can be uniquely identified across
  both protocols.

* BLE Device Name (READ, WRITE, NOTIFY; string). This is persisted to Flash
  (SPIFF). It's useful as a nickname, to identify multiple devices in a fleet
  management scenario. This is the name that appears in the Web BLE connection
  dialog.

The Service and Characteristic UUIDs are defined in
`./src/BLEMACAddressService.h`.

The **UART Service** used the Nordic UART Service and Characteristic UUDs. It
currently responds to RX "ping" with "pong", and "ping\n" with "pong\n".
It is for debugging and possible future extensions.

## Installation

1. Install [PlatformIO](https://platformio.org).
2. Install the Arduino ESP32 Board.
3. Install the "Adafruit BNO055" and "Adafruit Unified Sensor" libraries.
4. Build and upload the project.

This can all be done fairly easily by installing either the [PlatformIO Visual
Studio Code extension](https://platformio.org/install/ide?install=vscode), or
[PlatformIO for
Atom](https://docs.platformio.org/en/latest/ide/atom.html#installation), and
using the PlatformIO GUI within the editor.

It should also be possible to install the code using the [PlatformIO Command
Line](https://docs.platformio.org/en/latest/installation.html), or by opening
`main.cpp` in the Arduino IDE and install the Arduino ESP32 board.

## Portability

The code is currently specific to the ESP32. Porting it to another board that
supports the Arduino APIs requires at least these changes:

* The persistent configuration code uses SPIFF. If this is not available, use
  the Flash API.
* The BLE MAC address service publishes the WiFi MAC address. If there is no
  WiFi MAC address, use the BLE address.
* The BLE Device Name service uses an ESP IDF call set the BLE device name. Wrap
  this in an #ifdef. Maybe there is another way to do this on other boards? If
  not, some options are: reboot the board, or live with the fact that the device
  name change doesn't take effect until the user reboots the board.

## License

MIT
