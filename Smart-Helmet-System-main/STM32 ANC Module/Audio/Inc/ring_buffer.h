#ifndef AUDIO_INC_RING_BUFFER_H_
#define AUDIO_INC_RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Ring buffer size */
#define RING_BUFFER_SIZE 2048 

typedef struct {
    int16_t buffer[RING_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} RingBuffer_t;

/* Initialize the ring buffer */
void RingBuffer_Init(RingBuffer_t *rb);

/* Write data to the ring buffer */
bool RingBuffer_Write(RingBuffer_t *rb, const int16_t *data, uint32_t length);

/* Read data from the ring buffer */
bool RingBuffer_Read(RingBuffer_t *rb, int16_t *data, uint32_t length);

/* Get number of available samples to read */
uint32_t RingBuffer_GetCount(RingBuffer_t *rb);

/* Get free space in the buffer */
uint32_t RingBuffer_GetFreeSpace(RingBuffer_t *rb);

/* Clear the buffer */
void RingBuffer_Clear(RingBuffer_t *rb);

#endif /* AUDIO_INC_RING_BUFFER_H_ */
