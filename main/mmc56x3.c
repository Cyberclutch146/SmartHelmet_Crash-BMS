#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "mmc56x3.h"

////////////////////////////////////////////////////////////
// MMC56X3 I2C ADDRESS & REGISTERS
////////////////////////////////////////////////////////////

#define MMC56X3_ADDR            0x30

// Data output registers (X/Y/Z - 18-bit, 6 bytes)
#define MMC56X3_XOUT0           0x00
#define MMC56X3_XOUT1           0x01
#define MMC56X3_YOUT0           0x02
#define MMC56X3_YOUT1           0x03
#define MMC56X3_ZOUT0           0x04
#define MMC56X3_ZOUT1           0x05
#define MMC56X3_XYZOUT2         0x06    // bits [1:0] of each axis
#define MMC56X3_STATUS          0x18

// Control registers
#define MMC56X3_CTRL0           0x1B
#define MMC56X3_CTRL1           0x1C
#define MMC56X3_CTRL2           0x1D
#define MMC56X3_PRODUCT_ID      0x39

// Control bits
#define MMC56X3_CTRL0_SET       0x08
#define MMC56X3_CTRL0_RESET     0x10
#define MMC56X3_CTRL0_TM_M      0x01    // take measurement

#define MMC56X3_CTRL2_CMM_EN    0x10    // continuous mode
#define MMC56X3_CTRL2_CM_FREQ_100HZ  0x07  // ~100 Hz ODR
#define MMC56X3_CTRL2_EN_PRD_SET 0x80   // periodic SET

// Expected product ID
#define MMC56X3_PRODUCT_ID_VAL  0x10

// Sensitivity: 18-bit output covers ±8 Gauss = ±800 µT
// Counts per Gauss = 2^18 / 16 = 16384
// 1 Gauss = 100 µT → 1 count = 100/16384 µT ≈ 0.00610 µT
#define MMC56X3_UT_PER_COUNT    0.00610f
#define MMC56X3_OFFSET          131072.0f   // 2^17 midpoint

////////////////////////////////////////////////////////////
// MAGNETIC ANOMALY DETECTION STATE
////////////////////////////////////////////////////////////

#define MAG_ANOMALY_THRESHOLD   50.0f   // µT deviation
#define MAG_AVG_ALPHA           0.05f   // EMA smoothing

static float mag_avg_x = 0.0f;
static float mag_avg_y = 0.0f;
static float mag_avg_z = 0.0f;
static bool  mag_avg_initialized = false;

////////////////////////////////////////////////////////////
// INIT
////////////////////////////////////////////////////////////

bool mmc56x3_init(void)
{
    // Verify product ID
    uint8_t id = 0;
    i2c_read_bytes(MMC56X3_ADDR, MMC56X3_PRODUCT_ID, &id, 1);

    if (id != MMC56X3_PRODUCT_ID_VAL)
    {
        printf("[MAG] MMC56X3 not found (ID=0x%02X, "
               "expected 0x%02X)\n", id, MMC56X3_PRODUCT_ID_VAL);
        return false;
    }

    // Software reset via CTRL1
    i2c_write_reg(MMC56X3_ADDR, MMC56X3_CTRL1, 0x80);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Perform SET to magnetize the sensor
    i2c_write_reg(MMC56X3_ADDR, MMC56X3_CTRL0,
                  MMC56X3_CTRL0_SET);
    vTaskDelay(pdMS_TO_TICKS(1));

    // Enable continuous measurement mode at ~100 Hz
    // with periodic SET enabled for self-degauss
    uint8_t ctrl2_val = MMC56X3_CTRL2_CMM_EN
                      | MMC56X3_CTRL2_CM_FREQ_100HZ
                      | MMC56X3_CTRL2_EN_PRD_SET;

    i2c_write_reg(MMC56X3_ADDR, MMC56X3_CTRL2, ctrl2_val);

    printf("[MAG] MMC56X3 initialized (100 Hz, "
           "periodic SET)\n");
    return true;
}

////////////////////////////////////////////////////////////
// READ XYZ MAGNETIC FIELD (µT)
////////////////////////////////////////////////////////////

mag_data_t mmc56x3_read(void)
{
    mag_data_t result = {0};

    // Read 7 bytes: X0,X1,Y0,Y1,Z0,Z1,XYZ2
    uint8_t raw[7];
    i2c_read_bytes(MMC56X3_ADDR, MMC56X3_XOUT0, raw, 7);

    // Reconstruct 18-bit unsigned values
    // Byte layout:
    //   raw[0] = X[17:10], raw[1] = X[9:2]
    //   raw[6] bits [7:6] = X[1:0]
    //   raw[2] = Y[17:10], raw[3] = Y[9:2]
    //   raw[6] bits [5:4] = Y[1:0]
    //   raw[4] = Z[17:10], raw[5] = Z[9:2]
    //   raw[6] bits [3:2] = Z[1:0]

    uint32_t x_raw = ((uint32_t)raw[0] << 10)
                   | ((uint32_t)raw[1] << 2)
                   | ((raw[6] >> 6) & 0x03);

    uint32_t y_raw = ((uint32_t)raw[2] << 10)
                   | ((uint32_t)raw[3] << 2)
                   | ((raw[6] >> 4) & 0x03);

    uint32_t z_raw = ((uint32_t)raw[4] << 10)
                   | ((uint32_t)raw[5] << 2)
                   | ((raw[6] >> 2) & 0x03);

    // Convert to signed µT (offset binary → signed)
    result.x = ((float)x_raw - MMC56X3_OFFSET)
             * MMC56X3_UT_PER_COUNT;
    result.y = ((float)y_raw - MMC56X3_OFFSET)
             * MMC56X3_UT_PER_COUNT;
    result.z = ((float)z_raw - MMC56X3_OFFSET)
             * MMC56X3_UT_PER_COUNT;

    return result;
}

////////////////////////////////////////////////////////////
// TILT-COMPENSATED ORIENTATION (HEADING / PITCH / ROLL)
////////////////////////////////////////////////////////////

orientation_t mmc56x3_compute_orientation(
    mag_data_t mag,
    float ax, float ay, float az)
{
    orientation_t orient = {0};

    // ── Pitch & Roll from accelerometer ──
    orient.pitch = atan2f(-ax,
                          sqrtf(ay * ay + az * az));
    orient.roll  = atan2f(ay, az);

    float sp = sinf(orient.pitch);
    float cp = cosf(orient.pitch);
    float sr = sinf(orient.roll);
    float cr = cosf(orient.roll);

    // ── Tilt-compensated magnetic heading ──
    // Rotate magnetometer into horizontal plane
    float mag_x_h = mag.x * cp
                  + mag.y * sp * sr
                  + mag.z * sp * cr;

    float mag_y_h = mag.y * cr
                  - mag.z * sr;

    // Heading in degrees (0 = North, CW positive)
    float heading = atan2f(-mag_y_h, mag_x_h);
    heading = heading * (180.0f / M_PI);

    if (heading < 0.0f)
        heading += 360.0f;

    orient.heading = heading;

    // Convert pitch/roll to degrees
    orient.pitch *= (180.0f / M_PI);
    orient.roll  *= (180.0f / M_PI);

    return orient;
}

////////////////////////////////////////////////////////////
// MAGNETIC ANOMALY DETECTION
////////////////////////////////////////////////////////////

bool mmc56x3_detect_mag_anomaly(mag_data_t mag)
{
    if (!mag_avg_initialized)
    {
        mag_avg_x = mag.x;
        mag_avg_y = mag.y;
        mag_avg_z = mag.z;
        mag_avg_initialized = true;
        return false;
    }

    // Compute deviation from running average
    float dx = mag.x - mag_avg_x;
    float dy = mag.y - mag_avg_y;
    float dz = mag.z - mag_avg_z;
    float deviation = sqrtf(dx * dx + dy * dy + dz * dz);

    // Update EMA
    mag_avg_x += MAG_AVG_ALPHA * (mag.x - mag_avg_x);
    mag_avg_y += MAG_AVG_ALPHA * (mag.y - mag_avg_y);
    mag_avg_z += MAG_AVG_ALPHA * (mag.z - mag_avg_z);

    return (deviation > MAG_ANOMALY_THRESHOLD);
}
