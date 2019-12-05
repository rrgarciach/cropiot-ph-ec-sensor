# CropIoT Ph EC Sensor Device

An Arduino/NodeMCU compatible device to sense Ph and EC implementing CropIoT Device Settings Library.

## Installation using PlatformIO:
- Add CropIoT Device Settings library dependency at `platformio.ini` file:
```
lib_deps =
  https://github.com/rrgarciach/cropiot-device-settings.git
```
- Add include line in your `main.cpp`:
```
#include "CropIoTDeviceSettings.h"
```
- Declare the name of your device inside `setup()`. This name will be used as the Access Point name as well:
```
void setup() {
  DEVICE_TYPE = "my_device_name";
  ...
```
- Start Serial and run function to connect WiFi inside `setup()`:
```
  Serial.begin(115200);
  Serial.println("Starting...");

  connectWiFi();
```
- Generate any required WLAN:
```
  wlanClient1 = generateWiFiClient();
  connectMQTT(mqttClient1);
```

### Dependencies

- https://github.com/rrgarciach/cropiot-device-settings.git

### Additional hints
To upload static files from `/data` directory simply run:
`pio run -t uploadfs`

### Additional documentation
