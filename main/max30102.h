#pragma once

#include <stdint.h>

////////////////////////////////////////////////////////////
// MAX30102 PULSE OXIMETER DRIVER
////////////////////////////////////////////////////////////

void     max30102_init(void);

uint32_t max30102_read_ir(void);

int      max30102_compute_bpm(uint32_t *samples,
                              int count);
