#pragma once

////////////////////////////////////////////////////////////
// LSM6DSO IMU - ACCELEROMETER DRIVER
////////////////////////////////////////////////////////////

/**
 * Raw accelerometer reading (in g).
 */
typedef struct {
    float x;
    float y;
    float z;
} accel_data_t;

void lsm6dso_init(void);

float lsm6dso_read_accel_magnitude(void);

/**
 * Read X, Y, Z acceleration components (in g).
 * Needed by magnetometer for tilt-compensated heading.
 */
accel_data_t lsm6dso_read_accel_xyz(void);
