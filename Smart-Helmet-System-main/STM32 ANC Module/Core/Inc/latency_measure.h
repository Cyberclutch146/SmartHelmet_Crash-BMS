#ifndef CORE_INC_LATENCY_MEASURE_H_
#define CORE_INC_LATENCY_MEASURE_H_

#include <stdint.h>

void Latency_Init(void);
void Latency_Start(void);
uint32_t Latency_Stop(void);

#endif /* CORE_INC_LATENCY_MEASURE_H_ */
