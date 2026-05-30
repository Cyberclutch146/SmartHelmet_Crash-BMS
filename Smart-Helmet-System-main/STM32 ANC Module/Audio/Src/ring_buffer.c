#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t *rb) {
    if (rb == NULL) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool RingBuffer_Write(RingBuffer_t *rb, const int16_t *data, uint32_t length) {
    if (rb == NULL || data == NULL || length == 0) return false;
    
    /* Check if there is enough space */
    if (RingBuffer_GetFreeSpace(rb) < length) {
        return false; // Buffer overflow
    }
    
    for (uint32_t i = 0; i < length; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
    }
    
    rb->count += length;
    
    return true;
}

bool RingBuffer_Read(RingBuffer_t *rb, int16_t *data, uint32_t length) {
    if (rb == NULL || data == NULL || length == 0) return false;
    
    /* Check if there is enough data */
    if (RingBuffer_GetCount(rb) < length) {
        return false; // Buffer underflow
    }
    
    for (uint32_t i = 0; i < length; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    }
    
    rb->count -= length;
    
    return true;
}

uint32_t RingBuffer_GetCount(RingBuffer_t *rb) {
    if (rb == NULL) return 0;
    return rb->count;
}

uint32_t RingBuffer_GetFreeSpace(RingBuffer_t *rb) {
    if (rb == NULL) return 0;
    return RING_BUFFER_SIZE - rb->count;
}

void RingBuffer_Clear(RingBuffer_t *rb) {
    if (rb == NULL) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
