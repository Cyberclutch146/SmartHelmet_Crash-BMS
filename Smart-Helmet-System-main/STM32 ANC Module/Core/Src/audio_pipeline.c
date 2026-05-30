#include "audio_pipeline.h"
#include "pin_config.h"
#include "uart_parser.h"
#include "main.h"
#include "i2s.h"
#include "usart.h"
#include "dma.h"
#include <string.h>

int32_t mic_rx_buffer[DMA_BUFFER_SIZE];
int16_t amp_tx_buffer[DMA_BUFFER_SIZE];

RingBuffer_t Mic_RingBuffer;
RingBuffer_t Amp_RingBuffer;



void AudioPipeline_Init(void)
{
    memset(mic_rx_buffer, 0, sizeof(mic_rx_buffer));
    memset(amp_tx_buffer, 0, sizeof(amp_tx_buffer));

    RingBuffer_Init(&Mic_RingBuffer);
    RingBuffer_Init(&Amp_RingBuffer);

    UartParser_Init();


}

void AudioPipeline_Start(void)
{
    HAL_UART_Receive_DMA(&huart1, uart_dma_buffer, 4096); 
    HAL_I2S_Receive_DMA(&hi2s2, (uint16_t*)mic_rx_buffer, DMA_BUFFER_SIZE * 2);
    HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)amp_tx_buffer, DMA_BUFFER_SIZE);
}

void AudioPipeline_SetSampleRate(uint32_t sample_rate)
{
    uint32_t i2s_freq = I2S_AUDIOFREQ_44K;
    if (sample_rate == 48000) {
        i2s_freq = I2S_AUDIOFREQ_48K;
    }

    // Stop streams
    HAL_I2S_DMAStop(&hi2s2);
    HAL_I2S_DMAStop(&hi2s3);

    // Re-init with new freq
    HAL_I2S_DeInit(&hi2s2);
    HAL_I2S_DeInit(&hi2s3);
    
    hi2s2.Init.AudioFreq = i2s_freq;
    HAL_I2S_Init(&hi2s2);
    hi2s3.Init.AudioFreq = i2s_freq;
    HAL_I2S_Init(&hi2s3);

    // Clear buffers
    RingBuffer_Clear(&Mic_RingBuffer);
    RingBuffer_Clear(&Amp_RingBuffer);

    // Notify ANC engine (TODO: switch coefficients)
    // extern void ANC_SetSampleRate(uint32_t sr);
    // ANC_SetSampleRate(sample_rate);

    // Restart
    HAL_I2S_Receive_DMA(&hi2s2, (uint16_t*)mic_rx_buffer, DMA_BUFFER_SIZE * 2);
    HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)amp_tx_buffer, DMA_BUFFER_SIZE);
}

static void ProcessMicData(int startIndex) {
    int16_t extracted[AUDIO_CHUNK_SIZE];
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        // Extract 16-bit MSB from 32-bit I2S data
        extracted[i] = (int16_t)(mic_rx_buffer[startIndex + i] >> 16);
    }
    RingBuffer_Write(&Mic_RingBuffer, extracted, AUDIO_CHUNK_SIZE);
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == MIC_I2S) {
        ProcessMicData(0);
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == MIC_I2S) {
        ProcessMicData(AUDIO_CHUNK_SIZE);
    }
}

static void ProcessAmpData(int startIndex) {
    if (RingBuffer_GetCount(&Amp_RingBuffer) >= AUDIO_CHUNK_SIZE) {
        RingBuffer_Read(&Amp_RingBuffer, &amp_tx_buffer[startIndex], AUDIO_CHUNK_SIZE);
    } else {
        // Underflow: Output silence
        memset(&amp_tx_buffer[startIndex], 0, AUDIO_CHUNK_SIZE * sizeof(int16_t));
    }
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == AMP_I2S) {
        ProcessAmpData(0);
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == AMP_I2S) {
        ProcessAmpData(AUDIO_CHUNK_SIZE);
    }
}

