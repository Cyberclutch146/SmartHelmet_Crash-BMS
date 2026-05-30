#include <stdio.h>
#include <math.h>
#include "i2c_bus.h"
#include "lsm6dso.h"

////////////////////////////////////////////////////////////
// LSM6DSO REGISTERS & ADDRESS
////////////////////////////////////////////////////////////

#define LSM6DSO_ADDR    0x6B

#define LSM6DSO_CTRL1_XL    0x10
#define LSM6DSO_OUTX_L_A    0x28

// Sensitivity for ±16g range: 0.488 mg/LSB
#define LSM6DSO_SENSITIVITY 0.000488f

////////////////////////////////////////////////////////////
// INIT - 104 Hz, ±16g
////////////////////////////////////////////////////////////

void lsm6dso_init(void)
{
    // CTRL1_XL: ODR = 104 Hz (0x4_), FS = ±16g (_4)
    i2c_write_reg(LSM6DSO_ADDR, LSM6DSO_CTRL1_XL, 0x44);

    printf("[IMU] LSM6DSO initialized\n");
}

////////////////////////////////////////////////////////////
// READ ACCELERATION MAGNITUDE (in g)
////////////////////////////////////////////////////////////

float lsm6dso_read_accel_magnitude(void)
{
    uint8_t data[6];

    i2c_read_bytes(LSM6DSO_ADDR,
                   LSM6DSO_OUTX_L_A,
                   data,
                   6);

    int16_t ax = (data[1] << 8) | data[0];
    int16_t ay = (data[3] << 8) | data[2];
    int16_t az = (data[5] << 8) | data[4];

    float gx = ax * LSM6DSO_SENSITIVITY;
    float gy = ay * LSM6DSO_SENSITIVITY;
    float gz = az * LSM6DSO_SENSITIVITY;

    return sqrtf(gx * gx + gy * gy + gz * gz);
}

////////////////////////////////////////////////////////////
// READ ACCELERATION COMPONENTS (in g)
////////////////////////////////////////////////////////////

accel_data_t lsm6dso_read_accel_xyz(void)
{
    uint8_t data[6];

    i2c_read_bytes(LSM6DSO_ADDR,
                   LSM6DSO_OUTX_L_A,
                   data,
                   6);

    int16_t ax = (data[1] << 8) | data[0];
    int16_t ay = (data[3] << 8) | data[2];
    int16_t az = (data[5] << 8) | data[4];

    accel_data_t result;
    result.x = ax * LSM6DSO_SENSITIVITY;
    result.y = ay * LSM6DSO_SENSITIVITY;
    result.z = az * LSM6DSO_SENSITIVITY;

    return result;
}
