# automatic_watering_system

Arduino Uno R4 WiFi project that displays time, air humidity, and air temperature.

## Features
- Reads air humidity and temperature via Modulino Thermo
- Displays time (NTP), air humidity, and air temperature on an SSD1306 OLED

## Hardware
- Arduino Uno R4 WiFi
- Modulino Thermo (air humidity + temperature)
- SSD1306 OLED display (128×32)

## Setup
Add your WiFi credentials to `include/credentials.h`:
```cpp
#define WIFI_SSID "your_ssid"
#define WIFI_PASS "your_password"
```