# Arduino BLE IMU

This software runs on an ESP32 to publish BNO055 orientation data over a
Bluetooth Low Energy (BLE) connection, and optionally MQTT/WiFi, for use with
[osteele/imu-tools](https://github.com/osteele/imu-tools).

## BLE Services

The software defines the following BLE services.

### IMU Service (`509B0001-EBE1-4AA5-BC51-11004B78D5CB`)

Characteristics:

* Sensor (READ; `509B0002-EBE1-4AA5-BC51-11004B78D5CB`): A packed structure of
  sensors readings. The message format is:
  * Version (uint8): 1
  * Flags (uint8): specifies which of the following fields are present.
  * Time (uint16): the low 16 bits of the MCU's millisecond time
  * Quaternion (optional; 4 x float32): quaternion w, x, y, z
  * [Other sensors are not currently implemented]

  This protocol has the property that a message with just the Quaternion
  position is exactly 20 bytes, or one BLE 4.0 packet.

* Calibration (`509B0003-EBE1-4AA5-BC51-11004B78D5CB`; READ, NOTIFY; 4 x
  uint_8): The system, gyro, accel, and mag calibration values, each ranging
  0..3. These are NOTIFYed when they change (from initial assumed values of 0).

Characteristic data are transmitted in little-endian order, not network order.
This matches the standard GATT profile characteristics such as the heart-rate
characteristic.

The device is polled 60 times / second. The sensor characteristic is always
written and notified. The polling interval is a constant that is defined in the
code. The system doesn't appear to be capable of transmitting at greater than
~126 samples/second. For greater rates, consider MQTT, or onboard processing of
the data.

### MAC Address Service (`709F0001-37E3-439E-A338-23F00067988B`)

Characteristics:

* MAC address (`709F0002-37E3-439E-A338-23F00067988B`; READ; string). imu-tools
  uses the MAC address as a device id that persists across connections (the Web
  BLE API doesn't make the BLE device id available to code). The MAC address is
  used instead of the BLE address, so that a device that publishes both to WiFi
  and BLE can be uniquely identified across both protocols.

* BLE Device Name (`709F0003-37E3-439E-A338-23F00067988B`; READ, WRITE, NOTIFY;
  string). This is persisted to Flash via SPIFFS. It's useful as a nickname, to
  identify multiple devices in a fleet management scenario. This is the name
  that appears in the Web BLE connection dialog.

### UART Service

The **UART Service** (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`) uses the Nordic
UART Service and Characteristic UUIDs. It currently responds to RX "ping" with
"pong", and "ping\n" with "pong\n". It is for debugging and possible future
extensions.

Characteristics:

* RX (`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`; READ)
* TX (`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`; WRITE, NOTIFY)

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

## MQTT/WiFi

In order to publish MQTT messages, add the following [ESP
SPiFFS](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/spiffs.html)
files:

`wpa_supplicant.txt`

    ExampleSsid
    examplePassword

`mpqtt.config`

   m10.cloudmqtt.com
   1883
   username
   password

In the PlatformIO IDE, these can be added to `./data`. Then use Platformio: Run
Task... > Upload File System Image to copy these to the attached ESP's file
system. More information, and command-line instructions, are
[here](https://docs.platformio.org/en/latest/platforms/espressif32.html#uploading-files-to-file-system-spiffs).

The WPA supplicant may contain multiple (ssid, password) pairs, optionally
separated by a blank line. The device scans for WiFi stations, and a connection
is attempted to the first listed ssid in the supplicant file that is in this
scan. If this connection fails, another is not made, so an invalid password for
discovered ssid will prevent connection to subsequent networks.

Note that the ESP32 can't connect to 5 GHz WiFi networks.

## Portability

The code is currently specific to the ESP32. Porting it to another board that
supports the Arduino APIs requires at least these changes:

* The persistent configuration code uses SPIFFS. If this is not available, use
  the Flash API.
* The BLE MAC address service publishes the WiFi MAC address. If there is no
  WiFi MAC address, use the BLE address.
* The BLE Device Name service uses an ESP IDF call set the BLE device name. Wrap
  this in an #ifdef. Maybe there is another way to do this on other boards? If
  not, some options are: reboot the board, or live with the fact that the device
  name change doesn't take effect until the user reboots the board.

## License

MIT
