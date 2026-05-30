#ifndef AUDIO_INC_AUDIO_TYPES_H_
#define AUDIO_INC_AUDIO_TYPES_H_

#include <stdint.h>

#define AUDIO_CHUNK_SIZE 256 // Number of samples per processing block

typedef int16_t AudioSample_t; // Standard 16-bit audio sample

typedef struct {
    AudioSample_t left[AUDIO_CHUNK_SIZE];
    AudioSample_t right[AUDIO_CHUNK_SIZE];
} AudioBlock_t;

#endif /* AUDIO_INC_AUDIO_TYPES_H_ */
