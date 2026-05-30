#ifndef UART_PARSER_H
#define UART_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define BT_FIFO_SIZE 8192

extern uint8_t uart_dma_buffer[4096];

void UartParser_Init(void);
void UartParser_Process(void);
int UartParser_ReadSamples(int16_t *out_buffer, int num_samples);

#endif // UART_PARSER_H
