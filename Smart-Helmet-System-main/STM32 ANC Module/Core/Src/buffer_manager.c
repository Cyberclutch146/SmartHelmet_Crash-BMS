#include "buffer_manager.h"
#include "audio_pipeline.h"
#include "uart_parser.h"
#include "ring_buffer.h"
#include "anc_engine.h"

volatile bool is_anc_on = false; // False = Transparency, True = Prototype ANC

// ANC gain and tuning
#define ANC_GAIN 0.8f
#define TRANSPARENCY_GAIN 1.0f

void BufferManager_Process(void)
{
    // Wait until we have a full chunk of Mic data ready
    if (RingBuffer_GetCount(&Mic_RingBuffer) < AUDIO_CHUNK_SIZE) {
        return; // Not enough data yet
    }

    // Ensure we also have space in the AMP buffer before processing
    if (RingBuffer_GetFreeSpace(&Amp_RingBuffer) < AUDIO_CHUNK_SIZE * 2) {
        return; // Amp buffer is full, wait for DMA to drain it
    }

    int16_t mic_chunk[AUDIO_CHUNK_SIZE];
    int16_t out_chunk[AUDIO_CHUNK_SIZE * 2]; // Stereo interleaved

    // Read Mic data
    RingBuffer_Read(&Mic_RingBuffer, mic_chunk, AUDIO_CHUNK_SIZE);

    // Read Bluetooth data (if available, otherwise fill with 0)
    int16_t bt_buffer[AUDIO_CHUNK_SIZE * 2]; // Stereo
    int read_count = UartParser_ReadSamples(bt_buffer, AUDIO_CHUNK_SIZE * 2);
    
    // Pad with silence if not enough BT data
    for (int i = read_count; i < AUDIO_CHUNK_SIZE * 2; i++) {
        bt_buffer[i] = 0;
    }

    // Pass mic chunk through ANC engine to get anti-noise
    int16_t anti_noise_chunk[AUDIO_CHUNK_SIZE];
    ANC_ProcessBlock(mic_chunk, anti_noise_chunk, AUDIO_CHUNK_SIZE);

    // Process chunk
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++)
    {
        int16_t bt_l = bt_buffer[i * 2];
        int16_t bt_r = bt_buffer[i * 2 + 1];

        // Mix Bluetooth with generated Anti-Noise
        int32_t out_l = bt_l + anti_noise_chunk[i];
        int32_t out_r = bt_r + anti_noise_chunk[i];

        // Soft clip
        if (out_l > 32767) out_l = 32767;
        if (out_l < -32768) out_l = -32768;
        if (out_r > 32767) out_r = 32767;
        if (out_r < -32768) out_r = -32768;

        out_chunk[i * 2] = (int16_t)out_l;
        out_chunk[i * 2 + 1] = (int16_t)out_r;
    }

    // Push processed data to AMP ring buffer
    RingBuffer_Write(&Amp_RingBuffer, out_chunk, AUDIO_CHUNK_SIZE * 2);
}
