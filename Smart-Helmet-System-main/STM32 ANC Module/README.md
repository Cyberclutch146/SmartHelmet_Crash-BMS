# STM32 ANC & DSP Module - Technical Reference

This module is the core audio processing engine for the Smart Helmet System, running on an **STM32F401CCU6 (Black Pill)**. It handles real-time audio mixing, Bluetooth stream decoding, and Active Noise Cancellation (ANC) with zero CPU bottlenecks by heavily leveraging Direct Memory Access (DMA) hardware.

---

## 1. System Architecture overview

Because the STM32F401CCU6 only has two I2S peripherals, we cannot use I2S for the Bluetooth audio link. Instead, the architecture utilizes three separate high-speed serial streams:
1. **UART1 (RX Only)**: Receives a continuous 2 Mbps stream of packetized True Stereo audio from the ESP32.
2. **I2S2 / SPI2 (Master RX)**: Reads 32-bit I2S data from the INMP441 environmental microphone.
3. **I2S3 / SPI3 (Master TX)**: Transmits 16-bit mixed stereo audio to the MAX98357A AMP.

---

## 2. Hardware Pinout & Wiring

| Component | Signal | STM32 Pin | Alternate Function | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32 (BT Audio)** | UART RX | `PA10` | `AF7_USART1` | 2,000,000 Baud, 8N1 |
| **INMP441 (Mic)** | I2S WS | `PB12` | `AF5_SPI2` | Word Select (L/R clock) |
| | I2S SCK | `PB13` | `AF5_SPI2` | Bit Clock |
| | I2S SD | `PB15` | `AF5_SPI2` | Serial Data IN |
| **MAX98357A (AMP)** | I2S WS | `PA4` | `AF6_SPI3` | Word Select (L/R clock) |
| | I2S SCK | `PB3` | `AF6_SPI3` | Bit Clock |
| | I2S SD | `PB5` | `AF6_SPI3` | Serial Data OUT |
| **User Input** | Button | `PA0` | `GPIO_MODE_IT` | Pulled UP internally. Triggers on Falling Edge. |

---

## 3. Core Software Subsystems

The firmware is divided into three major components to ensure real-time audio deadlines are met:

### A. The Audio Pipeline (`audio_pipeline.c`)
This file is responsible for the bare-metal hardware configuration. 
- It bypasses the standard STM32CubeMX auto-generation to finely tune the DMA streams.
- **DMA Configuration**: 
  - `DMA2_Stream2_CH4`: Continuously dumps UART bytes into a `4096-byte` circular buffer in the background.
  - `DMA1_Stream3_CH0`: Pulls 32-bit I2S data from the Mic into a circular buffer.
  - `DMA1_Stream5_CH0`: Pushes 16-bit I2S data to the AMP.
- **The Heartbeat**: The entire software loop is driven by the I2S AMP DMA interrupts (`HAL_I2S_TxHalfCpltCallback` and `HAL_I2S_TxCpltCallback`). When the AMP needs more data, it flags the main loop to process the next chunk of audio.

### B. The UART Packet Parser (`uart_parser.c`)
Because UART is asynchronous, it is prone to data shifts if a byte is dropped. To guarantee True Stereo channel alignment, the ESP32 sends audio wrapped in packets.
- **Packet Structure**: `[0xAA, 0x55] (Sync Word)` + `[Length (16-bit)]` + `[Payload...]`
- **Lock-Free FIFO**: The `UartParser_Process()` runs in the main loop, scanning the DMA ring buffer for the `0xAA55` sync word. When found, it extracts the `int16_t` stereo samples and pushes them into an `8192-sample` lock-free FIFO queue (`bt_audio_fifo`).
- **Clock Drift Compensation**: The ESP32 and STM32 do not share a master clock. Over time, the ESP32 might send audio slightly slower or faster than the STM32 plays it. If the FIFO runs empty, the parser seamlessly duplicates the last known good sample to prevent loud static.

### C. The Buffer Manager & DSP Engine (`buffer_manager.c`)
This is where the actual audio math happens. Triggered by the DMA Half/Full callbacks, it processes audio in chunks of 256 samples.

#### Transparency Mode (Default)
When ANC is off, the system enters Transparency Mode. 
1. It pops Bluetooth audio from the FIFO.
2. It extracts the 16-bit MSB (Most Significant Bytes) from the 32-bit I2S microphone slot.
3. It adds the raw microphone audio directly to both the Left and Right Bluetooth channels, allowing the user to hear ambient surroundings (traffic, sirens, conversations) clearly while listening to music.

#### Prototype Active Noise Cancellation (ANC Mode)
When the user presses the toggle button (`PA0`), the EXTI interrupt flips the `is_anc_on` state.
1. The microphone audio is pushed into a `64-sample` circular delay buffer (providing a ~1.45ms phase delay at 44.1kHz).
2. The delayed microphone signal is **inverted** (multiplied by `-1`) and scaled by `ANC_GAIN` (0.8f).
3. This inverted, out-of-phase audio is added to the Bluetooth stream. When played through the speakers, it creates destructive interference with the physical sound waves penetrating the helmet, theoretically reducing constant low-frequency drones (like engine or wind noise).

---

## 4. Development & Compilation

To modify and build this firmware:
1. Open the project in **STM32CubeIDE**.
2. **Important**: Because the hardware configuration is written manually in `audio_pipeline.c`, you **do not** need to use the `.ioc` Device Configuration Tool. If you do open the `.ioc` file and click "Generate Code", it may overwrite parts of `main.c`. 
3. Build the project (Hammer Icon).
4. Flash the STM32 using an ST-Link V2 (Debug Icon).

## 5. Performance Metrics
- **CPU Load**: < 5% (Due to Circular DMA handling all memory transfers)
- **RAM Usage**: ~23 KB / 64 KB (16KB allocated to Bluetooth FIFO buffer to ensure maximum stability against UART jitter).
- **Latency**: < 6ms total system latency (vital for phase-accurate ANC).
