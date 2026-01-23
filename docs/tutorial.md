# Tutorial

## Overview
This project automates watering using soil moisture sensors, a pump and supporting modules. The system reads soil moisture and temperature, shows status on an OLED, sounds alerts with a buzzer, and waters plants using a pump and water rings. It is written for an Arduino-compatible board and uses PlatformIO for building and uploading.

## Project Files
[main.cpp](../src/main.cpp) — main firmware.<br>
[credentials.h](../include/credentials.h) — network / configurable secrets (add to include folder).
```h
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
```
[platformio.ini](../platformio.ini) — build/upload configuration.
## Hardware List
- Arduino UNO R4 WiFi
- Modulino THERMO
- Modulino BUZZER
- Soil moisture sensors (capacitive)
- OLED display (128x64 SSD1306)
- Breadboard
- Jumper wires
- Water pump
- Water rings
- Silicone hose
- Relay module or logic-level MOSFET, diode, and transistor components for pump control

## Wiring (recommended pin mapping)
Adjust pins in [main.cpp](../src/main.cpp) if your board differs.

- OLED (I2C)
    - SDA → Arduino SDA (A4 on Uno)
    - SCL → Arduino SCL (A5 on Uno)
    - VCC → 3.3V or 5V (per OLED spec)
    - GND → GND
- Modulino THERMO
    - Data → configured digital pin (see code)
- Modulino BUZZER
    - digital output (e.g., D6)
- Soil sensors
    - Signal → analog pins (e.g., A0, A1, A2)
    - VCC → 5V
    - GND → GND
- Pump
    - Pump positive → external power supply positive
    - Pump negative → Relay/MOSFET drain (or relay NO)
    - MOSFET source → GND (common ground with Arduino)
    - Gate → Arduino digital output (through 100–220Ω resistor)
    - Add flyback diode across pump if using DC motor driver or use a diode/RC snubber as appropriate
    - Power supply ground must be common with Arduino ground when using MOSFET control
- Water rings and hoses: connect pump output to tubing, install rings to plant pots

Safety notes:

Always use a separate power supply for the pump.
Add a fuse appropriate to the pump current.
Keep electronics away from water; mount pump and water fittings outside enclosures or use waterproof housings.
## Software setup
1. Install PlatformIO (VSCode recommended) or PlatformIO Core CLI.
2. Open the project folder.
3. Edit configuration/secrets:
    - Open credentials.h to set any Wi‑Fi or network values the project expects.
4. Select the correct board in platformio.ini if needed.

Build and upload with PlatformIO (CLI):
```bash
# Build
pio run

# Upload
pio run -t upload

# Monitor serial output
pio device monitor
```
Or use the PlatformIO UI in VSCode to Build, Upload and Monitor.

## Configuration & Calibration
- Soil sensor threshold: Find a comfortable threshold for your plants. Typical ranges:
    - Dry: > ~700 (depending on sensor)
    - Moist: ~300–600<br>
    Adjust <mark style="background-color:grey;">DRY_THRESHOLD</mark> and <mark style="background-color:grey;">WET_THRESHOLD</mark> in code (or constants) to suit your sensors.
- Calibrate by comparing sensor readings in dry air, in wet soil, and in water.
- Pump run duration: Set an amount of seconds per cycle to prevent overwatering. Adjust PUMP_DURATION_MS or similar in code.
- Set minimum interval between waterings to prevent continuous pump toggling.

## Calibration procedure:
1. Insert sensor into dry potting mix, record analog value.
2. Water fully, wait for equilibrium, record analog value.
3. Choose a midpoint (or plant-specific safe threshold) and update code.

## Operation
- On boot the OLED shows status and sensors readout.
- When moisture < threshold and minimum delay passed, the pump runs for configured duration.
- The buzzer sounds an alert for critical states (low water reservoir, sensor errors, etc.)

Testing without water:

- Test logic by simulating low-moisture values (or lift sensor out of soil) and watch pump activation.
- To avoid accidental water flow during test, disconnect pump power or test with an LED in place of pump.

## Troubleshooting
- Pump doesn't run:
    - Verify pump supply voltage and common ground.
    - Check MOSFET/relay wiring and driver part orientation.
    - Use a multimeter to check control pin outputs during activation.
- Sensors read strange values:
    - Check sensor wiring, clean probes, try different soil depths.
    - Some cheap sensors require pull-down/up hardware for stable readings.
- OLED blank:
    - Verify I2C address and SDA/SCL wiring; try i2cdetect on Linux with USB-serial bridge if applicable.
- Buzzer silent:
    - Confirm active vs passive buzzer type; passive buzzers need PWM signals.

## Maintenance
- Clean soil sensors periodically; salts and minerals build up on probes.
- Inspect hoses and rings for clogs or leaks.
- Replace water filters to protect the pump.
- Verify firmware periodically and backup configurations.

## Safety and Best Practices
- Never place the Arduino or open electronics where they can be splashed.
- Use waterproof enclosures or separate wet components physically.
- Ensure proper fusing and use GFCI where appropriate for AC-powered pumps.
- Add a water-level sensor in the reservoir to prevent dry-run damage to the pump.

## Extending the Project
- Add a reservoir level sensor and firmware check before pump activation.
- Implement a web UI or mobile alerts if networked.
- Add per-zone valves for multi-zone watering with multiple pumps or solenoids.
- Log sensor data to an SD card or cloud service for trends.

## Quick reference commands
```bash
# build
pio run

# upload
pio run -t upload

# serial monitor (adjust baud if needed)
pio device monitor -b 115200
```

## Where to edit behavior
- Main logic and thresholds: [src/main.cpp](../src/main.cpp)
- Secrets/config: [include/credentials.h](../include/credentials.h)
- Build config: [platformio.ini](../platformio.ini)