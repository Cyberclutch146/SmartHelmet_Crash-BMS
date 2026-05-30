#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "ring_buffer.h"

// 256 int16_t samples = 128 stereo frames (Left, Right)
#define AUDIO_CHUNK_SIZE 256

// Double buffer for DMA (Half-transfer and Full-transfer)
#define DMA_BUFFER_SIZE  (AUDIO_CHUNK_SIZE * 2)

extern int32_t mic_rx_buffer[DMA_BUFFER_SIZE];
extern int16_t amp_tx_buffer[DMA_BUFFER_SIZE];

extern RingBuffer_t Mic_RingBuffer;
extern RingBuffer_t Amp_RingBuffer;

void AudioPipeline_Init(void);
void AudioPipeline_Start(void);
void AudioPipeline_SetSampleRate(uint32_t sample_rate);

#endif // AUDIO_PIPELINE_H
