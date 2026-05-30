#pragma once
#include <stdint.h>

// UART configuration for ESP32 (using UART2 → TX2=GPIO17, RX2=GPIO16)
#define UART_STREAM_PORT UART_NUM_2
#define UART_STREAM_TX_PIN 17
#define UART_STREAM_RX_PIN 16
#define UART_STREAM_BAUD 2000000   // you can try 3000000 later

// Packet format
#define UART_SYNC_WORD 0xAA55
#define UART_FRAME_SAMPLES 256   // number of int16_t samples per packet

// Init UART
void uart_stream_init(void);

// Send interleaved stereo samples (int16_t)
// samples = number of int16_t values (L,R interleaved)
void uart_stream_send(int16_t *data, int samples);

// Send configuration packet
void uart_stream_send_config(uint32_t sample_rate);