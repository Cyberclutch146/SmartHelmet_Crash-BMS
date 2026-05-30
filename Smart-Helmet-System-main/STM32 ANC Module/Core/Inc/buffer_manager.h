/**
 * @file buffer_manager.h
 * @brief Audio processing and mixing engine.
 * 
 * This module takes the raw buffers filled by the audio pipeline DMA,
 * mixes the microphone signal with the Bluetooth audio, and handles
 * the toggle between Transparency mode and Active Noise Cancellation (ANC) mode.
 */
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

extern volatile bool is_anc_on;

void BufferManager_Process(void);

#endif // BUFFER_MANAGER_H
