# Smart Helmet Circuit Diagram & Wiring Guide

This document outlines the hardware connections required to build the Smart Helmet System, linking the ESP32 (Bluetooth Audio Receiver) with the STM32F401CCU6 (DSP/ANC Module).

## System Architecture

```mermaid
graph TD
    Phone[Mobile Phone] -- Bluetooth A2DP --> ESP32[ESP32 Communication Module]
    
    ESP32 -- UART 2Mbps --> STM32[STM32F401CCU6 Black Pill]
    
    Mic[INMP441 I2S Microphone] -- I2S Data --> STM32
    
    STM32 -- I2S Mixed Audio --> AMP_L[MAX98357A I2S Amplifier - Left]
    STM32 -- I2S Mixed Audio --> AMP_R[MAX98357A I2S Amplifier - Right]
    
    AMP_L -- Analog Audio --> SPK_L[Left Speaker]
    AMP_R -- Analog Audio --> SPK_R[Right Speaker]
    
    Button[ANC Toggle Button] -- GPIO --> STM32
```

## Detailed Pin Mappings

### 1. ESP32 to STM32 (High-Speed UART)
Used to transmit the decoded A2DP stereo audio to the DSP module.

| ESP32 Pin | STM32F401 Pin | Function |
| :--- | :--- | :--- |
| `GPIO17` (TX2) | `PA10` (UART1 RX) | High-speed (2 Mbps) audio stream |
| `GND` | `GND` | Common Ground Reference |

### 2. INMP441 Microphone to STM32 (Ambient Noise Input)
Used to capture environmental noise for Transparency Mode and ANC.

| INMP441 Pin | STM32F401 Pin | Function |
| :--- | :--- | :--- |
| `VDD` | `3.3V` | Power supply |
| `GND` | `GND` | Ground |
| `L/R` | `GND` | Sets mic to output on Left Channel |
| `WS` | `PB12` (I2S2 WS) | Word Select (Left/Right Clock) |
| `SCK` | `PB13` (I2S2 SCK) | Serial Clock |
| `SD` | `PB15` (I2S2 SD) | Serial Data Output |

### 3. MAX98357A Amplifiers to STM32 (Speaker Output)
Used to directly convert the digital I2S signal to amplified analog audio for the speakers. **Note: You need TWO of these modules wired in parallel for stereo.**

| MAX98357A Pin | STM32F401 Pin | Function |
| :--- | :--- | :--- |
| `VIN` | `5V` | Power supply for the amplifier |
| `GND` | `GND` | Ground |
| `LRC` | `PA4` (I2S3 WS) | Word Select (Left/Right Clock) |
| `BCLK` | `PB3` (I2S3 SCK) | Bit Clock |
| `DIN` | `PB5` (I2S3 SD) | Serial Data Input |
| `SD_MODE` (Left) | `5V` | Connect directly to VIN to output Left channel |
| `SD_MODE` (Right)| `5V` via 100kΩ Resistor | Connect to VIN through a 100kΩ resistor to output Right channel |

### 4. ANC Toggle Button
Used to switch between Transparency Mode (hear surroundings) and ANC Mode (cancel noise).

| Button Pin 1 | Button Pin 2 | Function |
| :--- | :--- | :--- |
| `PA0` | `GND` | Triggers EXTI interrupt on Falling Edge. STM32 configures internal Pull-Up. |

## Power Considerations
1. **Common Ground:** Ensure that the ESP32, STM32, Microphone, and AMP all share a common ground line to prevent noise and data corruption.
2. **Voltage Levels:** The STM32F401 and ESP32 are both 3.3V logic devices, so the UART and I2S lines can be connected directly without level shifters.
3. **Amplifier Power:** The MAX98357A amplifiers can draw significant current when driving speakers. Ensure your 5V power supply (like a USB power bank or dedicated 5V regulator) can handle at least 1A to 2A to prevent brownouts.