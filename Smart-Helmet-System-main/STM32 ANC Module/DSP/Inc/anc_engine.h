#ifndef DSP_INC_ANC_ENGINE_H_
#define DSP_INC_ANC_ENGINE_H_

#include "audio_types.h"
#include <stdbool.h>

void ANC_Init(void);
void ANC_SetMode(bool enable_anc);
void ANC_SetSampleRate(uint32_t sample_rate);
void ANC_ProcessBlock(const AudioSample_t* mic_in, AudioSample_t* anti_noise_out, uint32_t length);

#endif /* DSP_INC_ANC_ENGINE_H_ */
