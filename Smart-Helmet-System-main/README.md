# Smart Helmet System

This repository contains the firmware components for a Smart Helmet system featuring Bluetooth A2DP music streaming, Active Noise Cancellation (ANC), and Transparency modes.

## System Architecture

The system consists of two primary microcontrollers working in tandem:

### 1. Communication Module (ESP32)
Responsible for maintaining the Bluetooth classic connection with a mobile device, functioning as an A2DP sink.
- **Function**: Receives high-quality audio from the connected phone.
- **Output**: Streams interleaved 16-bit True Stereo PCM audio over a high-speed (2 Mbps) UART link to the DSP module.
- **Location**: `Communication Module ESP32[OLD]/`
- **Build System**: ESP-IDF

### 2. DSP & ANC Module (STM32F401CCU6 Black Pill)
Responsible for all audio processing, mixing, and output to the speakers.
- **Function**: Receives the True Stereo UART stream from the ESP32. Simultaneously captures ambient noise via an I2S INMP441 microphone.
- **Processing**: 
  - **Transparency Mode**: Mixes the ambient microphone sound directly into the stereo audio stream, allowing the user to hear their surroundings (ideal for safety while riding).
  - **ANC Mode**: Inverts and delays the ambient microphone sound to create destructive interference (prototype Active Noise Cancellation), reducing outside wind/traffic noise.
- **Output**: Outputs the mixed signal via I2S to a DAC (e.g., PCM5102) connected to the helmet's speakers.
- **Location**: `STM32 ANC Module/`
- **Build System**: STM32CubeIDE (using STM32 HAL)

## Hardware Pinout (STM32F401CCU6)

| Component | Function | STM32 Pin |
| :--- | :--- | :--- |
| **ESP32** | UART TX (Audio Stream) | `PA10` (UART1 RX) |
| **INMP441 Mic** | I2S WS (Word Select) | `PB12` (I2S2 WS) |
| | I2S SCK (Clock) | `PB13` (I2S2 SCK) |
| | I2S SD (Data) | `PB15` (I2S2 SD) |
| **PCM5102 DAC** | I2S WS | `PA4` (I2S3 WS) |
| | I2S SCK | `PB3` (I2S3 SCK) |
| | I2S SD | `PB5` (I2S3 SD) |
| **Push Button** | Toggle ANC / Transparency | `PA0` (Internal Pull-Up) |

## DSP Data Flow

1. **DMA RX**: The STM32 uses Circular DMA to constantly fill memory buffers with UART data (Bluetooth) and I2S2 data (Mic).
2. **Ping-Pong Buffering**: When the DMA completes half or all of a buffer, an interrupt triggers the `BufferManager_Process()` loop.
3. **Mixing Engine**: The Buffer Manager normalizes the 24-bit microphone data, applies gain, inverts it if ANC is enabled, and sums it with the 16-bit Bluetooth stereo stream.
4. **DMA TX**: The resulting mixed buffer is automatically streamed out to the DAC via I2S3 DMA.

## Building and Flashing

### Building the ESP32 Module
Ensure you have ESP-IDF v5.x installed.
```bash
cd "Communication Module ESP32[OLD]"
idf.py build
idf.py -p (PORT) flash monitor
```

### Building the STM32 Module
1. Open STM32CubeIDE.
2. Select File -> Import -> General -> Existing Projects into Workspace.
3. Select the `STM32 ANC Module` folder.
4. Click the "Build" hammer icon.
5. Flash to the Black Pill using an ST-Link v2 by clicking the "Debug" or "Run" icon.

*Note: The hardware configuration (Clocks, GPIO, DMA, UART, I2S) is written manually in `audio_pipeline.c`. Regenerating code using the `.ioc` file is not strictly necessary and may cause merge conflicts in `main.c`.*
