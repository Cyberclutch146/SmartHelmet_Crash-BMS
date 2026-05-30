#pragma once

////////////////////////////////////////////////////////////
// MMC56X3 MAGNETOMETER - SENSOR FUSION COMPLEMENT
////////////////////////////////////////////////////////////

#include <stdbool.h>

/**
 * Magnetometer reading in micro-Tesla (µT).
 */
typedef struct {
    float x;
    float y;
    float z;
} mag_data_t;

/**
 * Fused orientation derived from accel + mag data.
 * Heading = compass bearing (0-360 deg).
 * Pitch / Roll from accelerometer tilt.
 */
typedef struct {
    float heading;      // degrees (0-360)
    float pitch;        // degrees
    float roll;         // degrees
} orientation_t;

/**
 * Initialize MMC56X3: soft reset, SET/RESET,
 * configure 100 Hz continuous mode.
 * Returns true on success.
 */
bool mmc56x3_init(void);

/**
 * Read calibrated XYZ magnetic field (µT).
 */
mag_data_t mmc56x3_read(void);

/**
 * Compute heading (compass bearing) from
 * tilt-compensated magnetometer + accelerometer.
 *
 * @param mag   Latest magnetometer reading
 * @param ax    Accelerometer X (g)
 * @param ay    Accelerometer Y (g)
 * @param az    Accelerometer Z (g)
 * @return      Orientation with heading, pitch, roll
 */
orientation_t mmc56x3_compute_orientation(
    mag_data_t mag,
    float ax, float ay, float az);

/**
 * Detect sudden large change in magnetic field
 * that may indicate metallic impact / crash debris.
 * Uses a running average and checks deviation.
 *
 * @param mag   Latest magnetometer reading
 * @return      true if anomaly detected
 */
bool mmc56x3_detect_mag_anomaly(mag_data_t mag);
