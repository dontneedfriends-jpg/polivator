# Plant Moisture Sensor (Polivator)

## Features

- Measures soil moisture using capacitive sensor
- Displays moisture percentage and raw value on e-ink display
- Web interface for status and calibration
- Serial commands for calibration
- WiFi connectivity with captive portal for configuration
- Persistent calibration stored in NVS
- Deep sleep support (optional)

## Pin Connections (WeActStudio 2.13" e-ink on ESP32-S3)

| Display Pin | ESP32-S3 GPIO |
|-------------|---------------|
| CS          | 10            |
| DC          | 9             |
| RST         | 8             |
| BUSY        | 7             |
| MOSI        | 11            |
| SCK         | 12            |
| Sensor      | GPIO 4 (ADC)  |

## Build Instructions

1. Install PlatformIO (or use Arduino IDE)
2. Open this project in PlatformIO
3. Connect ESP32-S3 via USB
4. Build and upload: `pio run -t upload`
5. Monitor serial: `pio device monitor`

## Calibration

- Press 'c' via Serial to enter calibration mode
- Place sensor in dry soil (or air), then press 'd' to set dry value
- Place sensor in wet soil (or water), then press 'w' to set wet value
- Press 'r' to reset to defaults
- Calibration can also be done via web interface

## WiFi Configuration

- On first boot, the device creates an AP named "Polivator-Config"
- Connect to this AP and navigate to captive portal (or http://192.168.4.1)
- Enter your WiFi credentials and save
- Device will reboot and connect to your network

## Web Interface

- Status: `http://<device-ip>/api/status`
- Calibration: POST to `/api/calibrate/dry`, `/api/calibrate/wet`, `/api/calibrate/reset`
- Full UI planned for future

## Dependencies (managed by PlatformIO)

- GxEPD2
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
