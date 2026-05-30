# STM32CubeMX Configuration Guide: ANC Module

Based on the current codebase (`main.c`, `audio_pipeline.c`, `pin_config.h`, and `stm32f4xx_hal_msp.c`), here is the exact configuration you need to set up in **STM32CubeMX** for your STM32F401CCU6 (Black Pill) board.

## 1. System Core

### RCC (Reset and Clock Control)
- **High Speed Clock (HSE):** Disable (Your code uses HSI)
- **Low Speed Clock (LSE):** Disable
- **Master Clock Output:** Disable

### SYS (System)
- **Debug:** Serial Wire
- **Timebase Source:** SysTick

### Clock Configuration (Tab)
*Your code sets up a basic 16 MHz internal clock.*
- **System Clock Mux:** HSI
- **HCLK (MHz):** 16 MHz
- **APB1 & APB2 Prescaler:** /1
- **I2S Clocks:** Let CubeMX auto-resolve the I2S PLL or use default values (your code sets `I2S_CLOCK_PLL` internally).

---

## 2. Pinout & Peripherals

### GPIO (General Purpose Input/Output)
Configure the **ANC Toggle Button**:
- **Pin:** `PA0`
- **Mode:** GPIO_EXTI0 (External Interrupt Mode with Falling edge trigger detection)
- **Pull-up/Pull-down:** Pull-up
- **User Label:** `ANC_BTN_PIN`

### USART1 (Bluetooth Audio from ESP32)
- **Mode:** Receive Only
- **Hardware Flow Control:** Disable
- **Parameter Settings:**
  - **Baud Rate:** `2000000` Bits/s (2 Mbps)
  - **Word Length:** 8 Bits (Including Parity)
  - **Parity:** None
  - **Stop Bits:** 1
  - **Over Sampling:** 16 Samples
- **GPIO Settings:**
  - **RX Pin:** `PA10` (Alternate Function: `GPIO_AF7_USART1`)
- **DMA Settings:**
  - Add **USART1_RX**
  - **Stream:** DMA2 Stream 2
  - **Direction:** Peripheral To Memory
  - **Priority:** Medium
  - **Mode:** Circular
  - **Data Width:** Byte (Peripheral) / Byte (Memory)

### I2S2 (INMP441 Microphone RX)
- **Mode:** Master Receive
- **Parameter Settings:**
  - **I2S Standard:** Philips
  - **Data and Frame Format:** 32 Bits Data on 32 Bits Frame
  - **Audio Frequency:** 44.1 KHz (Code adjusts this dynamically to 48 KHz if needed)
  - **Clock Polarity:** Low
- **GPIO Settings:**
  - **I2S2_WS:** `PB12` (Alternate Function: `GPIO_AF5_SPI2`)
  - **I2S2_CK:** `PB13` (Alternate Function: `GPIO_AF5_SPI2`)
  - **I2S2_SD:** `PB15` (Alternate Function: `GPIO_AF5_SPI2`)
- **DMA Settings:**
  - Add **SPI2_RX**
  - **Stream:** DMA1 Stream 3
  - **Direction:** Peripheral To Memory
  - **Priority:** High
  - **Mode:** Circular
  - **Data Width:** Word (Peripheral) / Word (Memory)

### I2S3 (AMP MAX98357A TX)
- **Mode:** Master Transmit
- **Parameter Settings:**
  - **I2S Standard:** Philips
  - **Data and Frame Format:** 16 Bits Data on 16 Bits Frame
  - **Audio Frequency:** 44.1 KHz (Code adjusts this dynamically to 48 KHz if needed)
  - **Clock Polarity:** Low
- **GPIO Settings:**
  - **I2S3_WS:** `PA4` (Alternate Function: `GPIO_AF6_SPI3`)
  - **I2S3_CK:** `PB3` (Alternate Function: `GPIO_AF6_SPI3`)
  - **I2S3_SD:** `PB5` (Alternate Function: `GPIO_AF6_SPI3`)
- **DMA Settings:**
  - Add **SPI3_TX**
  - **Stream:** DMA1 Stream 5
  - **Direction:** Memory To Peripheral
  - **Priority:** Very High
  - **Mode:** Circular
  - **Data Width:** Half Word (Peripheral) / Half Word (Memory)

---

## 3. Interrupts (NVIC)

In the **System Core > NVIC** section (or the NVIC tab for each peripheral), enable the following and set their priorities:

| Interrupt Name | Enable | Preemption Priority |
| :--- | :---: | :---: |
| EXTI line0 interrupt | Yes | 2 |
| DMA2 stream2 global interrupt (USART1_RX) | Yes | 1 |
| DMA1 stream3 global interrupt (I2S2_RX) | Yes | 1 |
| DMA1 stream5 global interrupt (I2S3_TX) | Yes | 1 |

> **Priority Notes:** Your code relies on I2S and UART DMA interrupts having a higher priority (1) than the GPIO Button interrupt (2) so audio streaming isn't preempted by a button press.

## 4. Project Manager Settings

- **Code Generator Tab:**
  - Check **"Generate peripheral initialization as a pair of '.c/.h' files per peripheral"** to keep `main.c` clean.
  - *Note:* If you generate code from CubeMX, it may overwrite `main.c` or create a new `usart.c`/`i2s.c`/`dma.c`. Since you already have your own `audio_pipeline.c` managing these peripherals, you might just want to use CubeMX to verify your pinouts and clock tree rather than overwriting your custom files!


> **Hardware Wiring Note:** For stereo output with two MAX98357A amplifiers, connect both to the exact same I2S lines (PA4, PB3, PB5). Wire the SD_MODE pin of the Left channel amp to VDD, and the Right channel amp to VDD through a 100k ohm resistor.
