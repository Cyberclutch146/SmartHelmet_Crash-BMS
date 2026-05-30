/**
 * @file pin_config.h
 * @brief Hardware pin definitions for the STM32F401CCU6 Black Pill board.
 * 
 * This file defines all the physical pins used for UART RX, I2S Mic RX,
 * I2S DAC TX, and the ANC toggle button.
 */
#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include "stm32f4xx_hal.h"

// --------------------------------------------------------
// ANC Toggle Button
// --------------------------------------------------------
#define ANC_BTN_PIN       GPIO_PIN_0
#define ANC_BTN_PORT      GPIOA
#define ANC_BTN_IRQn      EXTI0_IRQn

// --------------------------------------------------------
// UART1 RX (Bluetooth Audio from ESP32)
// --------------------------------------------------------
#define BT_UART           USART1
#define BT_UART_RX_PIN    GPIO_PIN_10
#define BT_UART_PORT      GPIOA
#define BT_UART_AF        GPIO_AF7_USART1

// --------------------------------------------------------
// I2S2 (INMP441 Microphone RX)
// --------------------------------------------------------
#define MIC_I2S           SPI2
#define MIC_WS_PIN        GPIO_PIN_12
#define MIC_SCK_PIN       GPIO_PIN_13
#define MIC_SD_PIN        GPIO_PIN_15
#define MIC_I2S_PORT      GPIOB
#define MIC_I2S_AF        GPIO_AF5_SPI2

// --------------------------------------------------------
// I2S3 (AMP MAX98357A TX)
// --------------------------------------------------------
#define AMP_I2S           SPI3
#define AMP_WS_PIN        GPIO_PIN_4
#define AMP_WS_PORT       GPIOA
#define AMP_SCK_PIN       GPIO_PIN_3
#define AMP_SD_PIN        GPIO_PIN_5
#define AMP_I2S_PORT      GPIOB
#define AMP_I2S_AF        GPIO_AF6_SPI3

#endif // PIN_CONFIG_H
