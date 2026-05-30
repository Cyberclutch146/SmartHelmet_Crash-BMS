#include "anc_engine.h"
#include "fir_filter.h"
#include "latency_measure.h"

static bool current_anc_state = false;
static uint32_t current_sample_rate = 44100;

void ANC_Init(void) {
    FIR_Init();
    ANC_SetMode(false);
}

void ANC_SetMode(bool enable_anc) {
    current_anc_state = enable_anc;
    FIR_SetCoefficients(current_sample_rate, current_anc_state);
}

void ANC_SetSampleRate(uint32_t sample_rate) {
    current_sample_rate = sample_rate;
    FIR_SetCoefficients(current_sample_rate, current_anc_state);
}

void ANC_ProcessBlock(const AudioSample_t* mic_in, AudioSample_t* anti_noise_out, uint32_t length) {
    Latency_Start();
    
    // Pass microphone data through the FIR filter to generate anti-noise
    FIR_Process(mic_in, anti_noise_out, length);
    
    Latency_Stop(); // Stop counter
}
