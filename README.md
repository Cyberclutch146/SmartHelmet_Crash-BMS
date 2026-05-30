# 🏍️ Smart Helmet — Crash Detection & BMS System

A real-time crash detection system built on the **Seeed Studio XIAO ESP32S3** using ESP-IDF v6.0.1 and FreeRTOS. The helmet continuously monitors rider safety through multiple sensors and can automatically send an SOS alert to a companion phone app when a crash is confirmed.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Components](#hardware-components)
- [Wiring & Pinout](#wiring--pinout)
- [Software Architecture](#software-architecture)
- [Source File Reference](#source-file-reference)
- [Crash Detection Algorithm](#crash-detection-algorithm)
- [Wi-Fi Provisioning (SmartConfig)](#wi-fi-provisioning-smartconfig)
- [SOS Alert System](#sos-alert-system)
- [OLED Display](#oled-display)
- [Battery Monitoring](#battery-monitoring)
- [Two-ESP32 GPIO Handshake](#two-esp32-gpio-handshake)
- [Build & Flash](#build--flash)
- [Configuration](#configuration)
- [License](#license)

---

## Overview

When powered on, the helmet firmware launches **7 concurrent FreeRTOS tasks**, each responsible for a different aspect of rider safety:

| Task | Priority | Purpose |
|------|----------|---------|
| **IMU** | 4 (highest) | Reads accelerometer at 100 Hz to detect sudden impacts |
| **Barometer** | 3 | Monitors altitude for rapid drops (fall detection) |
| **Magnetometer** | 3 | Detects magnetic field anomalies from metal-on-metal collisions |
| **Stillness** | 2 | Checks if rider remains motionless after impact |
| **Supervisor** | 2 | Correlates all sensor flags to confirm crash and dispatch SOS |
| **Pulse** | 1 | Reads heart rate and body temperature periodically |
| **Display** | 1 | Updates OLED with live status (online, crash, battery) |

---

## Hardware Components

| Component | Model | Interface | Purpose |
|-----------|-------|-----------|---------|
| Microcontroller | Seeed XIAO ESP32S3 | — | Main processor |
| IMU (Accel + Gyro) | LSM6DSO | I2C | Impact / shock detection |
| Barometer | BMP581 | I2C | Altitude drop detection |
| Magnetometer | MMC56X3 | I2C | Magnetic anomaly / heading |
| Pulse Oximeter | MAX30102 | I2C | Heart rate (BPM) |
| Temperature Sensor | TMP117 | I2C | Body / ambient temperature |
| OLED Display | SSD1306 0.96" | I2C | Status display |
| Battery | 2S 18650 Li-ion Pack | — | Power supply (6.4V–8.4V) |
| BMS | LX LISC V2 | — | Battery protection |
| Buck Converter | Any 5V step-down | — | Regulates 8.4V → 5V for XIAO |
| Voltage Sensor | DC Voltage Divider | ADC | Reads battery voltage for % |

---

## Wiring & Pinout

### I2C Bus (All Sensors + OLED)

All I2C devices share the same bus in parallel:

| Sensor/OLED Pin | XIAO Pin |
|:---|:---|
| VCC | **3V3** |
| GND | **GND** |
| SDA | **D4** (GPIO 5) |
| SCL | **D5** (GPIO 6) |

### UART (Serial Communication)

| UART Module Pin | XIAO Pin |
|:---|:---|
| RX (of receiver) | **D6** (GPIO 43) — TX out |
| TX (of receiver) | **D7** (GPIO 44) — RX in |
| GND | **GND** |

### Battery Voltage Sensor (ADC)

| Voltage Divider Pin | Connection |
|:---|:---|
| High-side (VCC) | Battery BMS **P+** (up to 8.4V) |
| GND | Common **GND** |
| Signal / Output | **D3** (GPIO 4) |

> **DIY Divider:** 30kΩ from P+ → D3, then 10kΩ from D3 → GND. This gives a ratio of 4.0×, mapping 8.4V → ~2.1V (safe for the ESP32 ADC).

### GPIO Crash Handshake (Two-ESP32 System)

| XIAO Pin | Direction | Purpose |
|:---|:---|:---|
| **D1** (GPIO 2) | OUTPUT | Goes HIGH when crash detected → alerts secondary ESP32 |
| **D2** (GPIO 3) | INPUT (pull-down) | Reads HIGH from secondary ESP32 → cancels the crash alert |

### Power System

```
2S 18650 Battery  →  BMS (LX LISC V2)  →  Buck Converter (8.4V → 5V)  →  XIAO 5V pin
                                        →  Voltage Divider  →  XIAO D3 (ADC)
```

---

## Software Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        app_main()                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  GPIO Config  │  Wi-Fi Init  │  I2C Init  │  UART   │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                   │
│     ┌──────────┬────────┼────────┬─────────┬──────────┐     │
│     ▼          ▼        ▼        ▼         ▼          ▼     │
│  imu_task  baro_task  mag_task  still_task  pulse_task display│
│  (100Hz)   (10Hz)     (50Hz)   (5Hz)       (periodic)  (1Hz)│
│     │          │        │        │                           │
│     ▼          ▼        ▼        ▼                           │
│  shock_flag  alt_flag  mag_flag  still_flag                  │
│     │          │        │        │                           │
│     └──────────┴────────┴────────┘                           │
│                    │                                         │
│                    ▼                                         │
│             supervisor_task                                  │
│         ┌──────────────────────┐                             │
│         │ IF shock + stillness │                             │
│         │  → GPIO D1 HIGH     │                             │
│         │  → Wait 10s         │                             │
│         │  → Check D2 cancel  │                             │
│         │  → If no cancel:    │                             │
│         │     HTTP POST SOS   │                             │
│         │     UART packet     │                             │
│         └──────────────────────┘                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Source File Reference

| File | Purpose |
|------|---------|
| `main.c` | Entry point. Initializes all peripherals, creates all FreeRTOS tasks, contains the supervisor crash-decision logic. |
| `i2c_bus.c/h` | Shared I2C bus driver with mutex protection for thread-safe multi-sensor access. |
| `lsm6dso.c/h` | IMU driver. Reads accelerometer magnitude for impact detection. |
| `bmp581.c/h` | Barometric pressure sensor. Converts pressure to altitude and tracks drops. |
| `mmc56x3.c/h` | Magnetometer driver. Computes heading/orientation and detects magnetic anomalies. |
| `max30102.c/h` | Pulse oximeter. Reads IR samples and computes BPM. |
| `tmp117.c/h` | High-accuracy temperature sensor driver. |
| `display_oled.c/h` | SSD1306 OLED driver with built-in 5×7 font and 2× scaled text rendering. |
| `battery_monitor.c/h` | ADC-based battery voltage reader. Maps 2S Li-ion voltage (6.4–8.4V) to 0–100%. |
| `network_sos.c/h` | Wi-Fi (SmartConfig) and HTTP client. Sends JSON SOS alerts to a backend server. |
| `uart_comm.c/h` | UART serial packet transmitter for crash/pulse data. |
| `CMakeLists.txt` | Build configuration. Lists all source files and ESP-IDF component dependencies. |

---

## Crash Detection Algorithm

The system uses a **multi-sensor fusion** approach. A crash is only confirmed when multiple independent sensors agree:

### Step 1: Impact Detection (IMU)
The `imu_task` reads the LSM6DSO accelerometer at 100 Hz (every 10ms). It computes the total acceleration magnitude (√(x² + y² + z²)):

- **> 14g** → `shock_flag = 1` (severe impact)
- **> 8g** → `warning_flag = 1` (minor impact / pothole)

### Step 2: Altitude Drop (Barometer)
The `barometer_task` reads BMP581 pressure every 100ms and converts it to altitude. If the altitude drops by more than **2 meters** from the rolling baseline:

- `altitude_flag = 1`

### Step 3: Magnetic Anomaly (Magnetometer)
The `magnetometer_task` reads MMC56X3 every 20ms. If a sudden spike in magnetic field strength is detected (indicating metal collision):

- `mag_anomaly_flag = 1`

### Step 4: Post-Impact Stillness
The `stillness_task` waits **5 seconds** after a shock event. If the rider has not resumed normal movement:

- `stillness_flag = 1`

### Step 5: Supervisor Decision
The `supervisor_task` checks every 200ms. When **both** `shock_flag` AND `stillness_flag` are set:

1. Sets GPIO D1 **HIGH** to alert the secondary ESP32
2. Starts a **10-second countdown**, checking GPIO D2 every 100ms
3. If D2 goes HIGH (secondary ESP32 confirms rider is OK) → **Cancel** the alert
4. If 10 seconds pass with no cancellation → **Send SOS** via HTTP POST and UART

---

## Wi-Fi Provisioning (SmartConfig)

The helmet does **not** use hardcoded Wi-Fi credentials. Instead, it uses Espressif's **SmartConfig (EspTouch)** protocol to receive the Wi-Fi SSID and password wirelessly from the user's phone.

### How It Works

1. On boot, `network_wifi_init()` initializes the Wi-Fi stack in Station mode
2. A `smartconfig_task` launches in the background and starts listening
3. The user opens the **EspTouch** app on their phone (available for iOS and Android)
4. The user enters their Wi-Fi password and taps "Broadcast"
5. The helmet receives the credentials, connects to Wi-Fi, and saves them to NVS (non-volatile storage)
6. On subsequent boots, the helmet connects automatically without needing the app again

### Supported Apps
- **Espressif EspTouch** — [Android](https://play.google.com/store/apps/details?id=com.khoazero123.iot_esptouch_demo) / iOS
- Any custom app integrating the Espressif SmartConfig SDK

---

## SOS Alert System

When a crash is confirmed and not cancelled within 10 seconds, the system sends an HTTP POST request to a backend server.

### Endpoint
```
POST http://<SERVER_IP>:5000/api/sos
```

### JSON Payload
```json
{
  "crash": true,
  "battery": 72,
  "temperature": 32.5
}
```

### Configuring the Server URL
Edit the `SOS_API_URL` macro in `network_sos.c`:
```c
#define SOS_API_URL "http://192.168.1.100:5000/api/sos"
```

The companion app should run an HTTP server on the same Wi-Fi network that listens for this POST request and triggers notifications to emergency contacts.

---

## OLED Display

The 0.96" SSD1306 OLED shows three lines of real-time status:

```
┌──────────────────────┐
│ STATUS: ONLINE       │  ← Normal 1x text (Wi-Fi status)
│                      │
│ C R A S H : N O      │  ← 2x scaled text (Crash flag)
│                      │
│ B A T T : 8 5 %      │  ← 2x scaled text (Battery %)
│                      │
│                      │
└──────────────────────┘
```

- **ONLINE / OFFLINE**: Reflects real-time Wi-Fi connection status
- **CRASH: YES / NO**: Lights up when any crash flag is active
- **BATT: XX%**: Live battery percentage from the voltage sensor

The display uses a custom-rendered 5×7 bitmap font supporting uppercase letters, numbers, and common symbols. The crash and battery lines use a pixel-doubled 2× renderer for high visibility.

---

## Battery Monitoring

The `battery_monitor` module reads the 2S Li-ion battery voltage through a voltage divider connected to ADC1 Channel 3 (GPIO 4).

### Voltage → Percentage Mapping

| Battery State | Voltage | Percentage |
|:---|:---|:---|
| Fully Charged | 8.4V | 100% |
| Nominal | 7.4V | 50% |
| Empty (cutoff) | 6.4V | 0% |

The mapping is linear between 6.4V and 8.4V. ESP32 ADC calibration (curve fitting) is used when available for improved accuracy.

### Voltage Divider Calculation

```
V_adc = V_battery × R2 / (R1 + R2)
      = 8.4V × 10kΩ / (30kΩ + 10kΩ)
      = 2.1V  ← Safe for ESP32 ADC (max 3.1V)

VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2 = 4.0
```

---

## Two-ESP32 GPIO Handshake

The system supports a secondary ESP32 that acts as a **crash cancellation device** (e.g., a button on the handlebar or a buzzer unit).

### Communication Flow

```
XIAO ESP32S3 (Helmet)              Secondary ESP32
──────────────────────              ───────────────
       D1 (GPIO 2) ────────────────► GPIO Input
       [OUTPUT: HIGH when crash]     [Reads HIGH = crash detected]

       D2 (GPIO 3) ◄──────────────── GPIO Output
       [INPUT: reads cancel signal]  [Sets HIGH = rider pressed cancel]
```

1. **Crash Detected** → XIAO sets D1 HIGH
2. Secondary ESP32 reads D1 HIGH → alerts the rider (buzzer/LED)
3. Rider presses a cancel button on the secondary ESP32
4. Secondary ESP32 sets its output HIGH → XIAO reads D2 HIGH
5. XIAO sees cancellation within 10 seconds → **aborts the SOS**
6. D1 is reset to LOW

> **Note:** D2 (GPIO 3) has an internal pull-down resistor enabled to prevent false-positive cancellations from electrical noise.

---

## Build & Flash

### Prerequisites

- **ESP-IDF v6.0.1** installed and configured
- **Target:** ESP32-S3 (Seeed XIAO)
- ESP-IDF environment sourced (use the ESP-IDF Command Prompt or run `export.ps1`)

### Commands

```bash
# Set the target chip (only needed once)
idf.py set-target esp32s3

# Build the project
idf.py build

# Flash to the XIAO (adjust COM port as needed)
idf.py -p COM5 flash

# Monitor serial output
idf.py -p COM5 monitor

# Build + Flash + Monitor in one command
idf.py -p COM5 flash monitor
```

### First Boot Checklist

1. Flash the firmware to the XIAO
2. Open the serial monitor — you should see `HELMET ENGINE STARTED`
3. The OLED will show `STATUS: OFFLINE`
4. Open the **EspTouch** app on your phone
5. Enter your Wi-Fi password and tap Broadcast
6. The serial monitor will print `SmartConfig Got SSID and password from Phone!`
7. The OLED will change to `STATUS: ONLINE`
8. The helmet is now fully operational!

---

## Configuration

Key constants you may want to adjust are located in these files:

| Setting | File | Default | Description |
|:---|:---|:---|:---|
| `ALTITUDE_DROP_THRESHOLD` | `main.c` | 2.0m | Minimum altitude drop to trigger flag |
| Impact threshold (severe) | `main.c` | 14.0g | Acceleration magnitude for crash |
| Impact threshold (warning) | `main.c` | 8.0g | Acceleration magnitude for warning |
| Stillness delay | `main.c` | 5000ms | Time to wait before checking stillness |
| Cancellation window | `main.c` | 10s | Time rider has to cancel the SOS |
| `SOS_API_URL` | `network_sos.c` | `http://192.168.1.100:5000/api/sos` | Backend server endpoint |
| `VOLTAGE_DIVIDER_RATIO` | `battery_monitor.c` | 4.0 | Resistor divider ratio |
| `BATT_MAX_VOLTAGE` | `battery_monitor.c` | 8.4V | Full charge voltage (2S) |
| `BATT_MIN_VOLTAGE` | `battery_monitor.c` | 6.4V | Empty voltage (2S) |
| `I2C_BUS_SDA` | `i2c_bus.h` | GPIO 5 (D4) | I2C data pin |
| `I2C_BUS_SCL` | `i2c_bus.h` | GPIO 6 (D5) | I2C clock pin |
| `UART_COMM_TX` | `uart_comm.h` | GPIO 43 (D6) | UART transmit pin |
| `UART_COMM_RX` | `uart_comm.h` | GPIO 44 (D7) | UART receive pin |

---

## License

This project is open-source. Feel free to modify and distribute.
