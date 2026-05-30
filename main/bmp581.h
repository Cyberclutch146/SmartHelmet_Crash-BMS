#pragma once

#include <stdbool.h>

////////////////////////////////////////////////////////////
// BMP581 BAROMETER DRIVER
////////////////////////////////////////////////////////////

bool  bmp581_init(void);

bool  bmp581_read(float *pressure_pa,
                  float *temperature_c);

float bmp581_pressure_to_altitude(float pressure_pa);
