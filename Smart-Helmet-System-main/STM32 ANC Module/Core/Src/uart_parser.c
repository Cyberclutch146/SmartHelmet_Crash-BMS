#include "uart_parser.h"
#include "audio_pipeline.h"
#include "ring_buffer.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define UART_SYNC_WORD_AUDIO  0xAA55
#define UART_SYNC_WORD_CONFIG 0xAA56
#define UART_DMA_BUF_SIZE     4096

uint8_t uart_dma_buffer[UART_DMA_BUF_SIZE];
uint32_t uart_process_ptr = 0;

RingBuffer_t Uart_RingBuffer;

extern DMA_HandleTypeDef hdma_usart1_rx;

void UartParser_Init(void) {
    memset(uart_dma_buffer, 0, sizeof(uart_dma_buffer));
    RingBuffer_Init(&Uart_RingBuffer);
    uart_process_ptr = 0;
}

// Called from main loop
void UartParser_Process(void) {
    // Determine how many bytes DMA has written
    uint32_t dma_curr_ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
    uint32_t dma_write_ptr = UART_DMA_BUF_SIZE - dma_curr_ndtr;

    while (uart_process_ptr != dma_write_ptr) {
        uint32_t avail = (dma_write_ptr >= uart_process_ptr) ? 
                         (dma_write_ptr - uart_process_ptr) : 
                         (UART_DMA_BUF_SIZE - uart_process_ptr + dma_write_ptr);
                         
        if (avail < 4) break; // Not enough for header

        uint8_t b0 = uart_dma_buffer[uart_process_ptr];
        uint8_t b1 = uart_dma_buffer[(uart_process_ptr + 1) % UART_DMA_BUF_SIZE];
        uint16_t sync = b0 | (b1 << 8);

        if (sync == UART_SYNC_WORD_AUDIO || sync == UART_SYNC_WORD_CONFIG) {
            uint8_t l0 = uart_dma_buffer[(uart_process_ptr + 2) % UART_DMA_BUF_SIZE];
            uint8_t l1 = uart_dma_buffer[(uart_process_ptr + 3) % UART_DMA_BUF_SIZE];
            uint16_t len = l0 | (l1 << 8); // length in bytes

            // Sanity check length
            if (len > 2048) {
                // Invalid length, probably lost sync
                uart_process_ptr = (uart_process_ptr + 1) % UART_DMA_BUF_SIZE;
                continue;
            }

            if (avail < 4 + len) {
                break; // wait for full packet
            }

            // Packet is complete, parse data
            if (sync == UART_SYNC_WORD_AUDIO) {
                // We'll write to ring buffer in chunks to avoid large intermediate buffers
                // Since data is interleaved 16-bit, len should be even.
                int16_t sample;
                for (int i = 0; i < len; i += 2) {
                    uint8_t d0 = uart_dma_buffer[(uart_process_ptr + 4 + i) % UART_DMA_BUF_SIZE];
                    uint8_t d1 = uart_dma_buffer[(uart_process_ptr + 4 + i + 1) % UART_DMA_BUF_SIZE];
                    sample = (int16_t)(d0 | (d1 << 8));
                    RingBuffer_Write(&Uart_RingBuffer, &sample, 1);
                }
            } else if (sync == UART_SYNC_WORD_CONFIG) {
                if (len == 4) {
                    uint8_t d0 = uart_dma_buffer[(uart_process_ptr + 4) % UART_DMA_BUF_SIZE];
                    uint8_t d1 = uart_dma_buffer[(uart_process_ptr + 5) % UART_DMA_BUF_SIZE];
                    uint8_t d2 = uart_dma_buffer[(uart_process_ptr + 6) % UART_DMA_BUF_SIZE];
                    uint8_t d3 = uart_dma_buffer[(uart_process_ptr + 7) % UART_DMA_BUF_SIZE];
                    uint32_t sample_rate = d0 | (d1 << 8) | (d2 << 16) | (d3 << 24);
                    
                    AudioPipeline_SetSampleRate(sample_rate);
                }
            }

            uart_process_ptr = (uart_process_ptr + 4 + len) % UART_DMA_BUF_SIZE;
        } else {
            uart_process_ptr = (uart_process_ptr + 1) % UART_DMA_BUF_SIZE;
        }
    }
}

int UartParser_ReadSamples(int16_t *out_buffer, int num_samples) {
    if (RingBuffer_GetCount(&Uart_RingBuffer) >= num_samples) {
        RingBuffer_Read(&Uart_RingBuffer, out_buffer, num_samples);
        return num_samples;
    }
    
    // Read whatever is available
    int avail = RingBuffer_GetCount(&Uart_RingBuffer);
    if (avail > 0) {
        RingBuffer_Read(&Uart_RingBuffer, out_buffer, avail);
    }
    return avail;
}
