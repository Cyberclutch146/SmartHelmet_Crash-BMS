#ifndef DSP_INC_FIR_FILTER_H_
#define DSP_INC_FIR_FILTER_H_

#include <stdint.h>
#include <stdbool.h>

void FIR_Init(void);
void FIR_SetCoefficients(uint32_t sample_rate, bool is_anc_on);
void FIR_Process(const int16_t* input, int16_t* output, uint32_t length);

#endif /* DSP_INC_FIR_FILTER_H_ */
