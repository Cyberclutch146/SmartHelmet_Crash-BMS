#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "bmp581.h"

////////////////////////////////////////////////////////////
// BMP581 I2C ADDRESS & REGISTERS
////////////////////////////////////////////////////////////

#define BMP581_ADDR             0x47

#define BMP581_REG_CHIP_ID      0x01
#define BMP581_REG_TEMP_XLSB    0x1D
#define BMP581_REG_PRESS_XLSB   0x20
#define BMP581_REG_INT_STATUS   0x27
#define BMP581_REG_OSR_CONFIG   0x36
#define BMP581_REG_ODR_CONFIG   0x37
#define BMP581_REG_CMD          0x7E

#define BMP581_CHIP_ID_VAL      0x50

// Soft-reset command
#define BMP581_CMD_SOFTRESET    0xB6

// ODR_CONFIG power mode bits [1:0]
#define BMP581_PWR_STANDBY      0x00
#define BMP581_PWR_NORMAL       0x01
#define BMP581_PWR_FORCED       0x02

// INT_STATUS bit for data ready
#define BMP581_INT_DRDY         0x01

// Sea-level pressure for altitude calc (Pa)
#define SEA_LEVEL_PRESSURE      101325.0f

////////////////////////////////////////////////////////////
// INIT
////////////////////////////////////////////////////////////

bool bmp581_init(void)
{
    // Soft-reset
    i2c_write_reg(BMP581_ADDR,
                  BMP581_REG_CMD,
                  BMP581_CMD_SOFTRESET);

    vTaskDelay(pdMS_TO_TICKS(10));

    // Verify chip ID
    uint8_t chip_id = 0;
    i2c_read_bytes(BMP581_ADDR,
                   BMP581_REG_CHIP_ID,
                   &chip_id,
                   1);

    if (chip_id != BMP581_CHIP_ID_VAL)
    {
        printf("[BARO] BMP581 not found! "
               "ID=0x%02X (expected 0x%02X)\n",
               chip_id,
               BMP581_CHIP_ID_VAL);
        return false;
    }

    // Configure oversampling:
    // bit 6 = press_en = 1
    // osr_t [5:3] = 2 (4x temp)
    // osr_p [2:0] = 4 (16x pressure)
    // => (1 << 6) | (2 << 3) | 4 = 0x54
    i2c_write_reg(BMP581_ADDR,
                  BMP581_REG_OSR_CONFIG,
                  0x54);

    printf("[BARO] BMP581 initialized "
           "(ID=0x%02X)\n",
           chip_id);

    return true;
}

////////////////////////////////////////////////////////////
// READ PRESSURE & TEMPERATURE
////////////////////////////////////////////////////////////

bool bmp581_read(float *pressure_pa,
                 float *temperature_c)
{
    // Trigger forced-mode measurement
    // ODR_CONFIG: pwr_mode[1:0] = forced (0x02)
    i2c_write_reg(BMP581_ADDR,
                  BMP581_REG_ODR_CONFIG,
                  BMP581_PWR_FORCED);

    // Wait for measurement to complete
    // With 4x temp + 16x pressure OSR,
    // typical conversion ~20ms
    vTaskDelay(pdMS_TO_TICKS(25));

    // Check data-ready via INT_STATUS
    uint8_t status = 0;
    i2c_read_bytes(BMP581_ADDR,
                   BMP581_REG_INT_STATUS,
                   &status,
                   1);

    if (!(status & BMP581_INT_DRDY))
    {
        // Data not ready; wait a bit more
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Read temperature (3 bytes at 0x1D)
    uint8_t temp_data[3];
    i2c_read_bytes(BMP581_ADDR,
                   BMP581_REG_TEMP_XLSB,
                   temp_data,
                   3);

    // Read pressure (3 bytes at 0x20)
    uint8_t press_data[3];
    i2c_read_bytes(BMP581_ADDR,
                   BMP581_REG_PRESS_XLSB,
                   press_data,
                   3);

    // Convert raw 24-bit temperature
    // BMP581 output: signed 24-bit, LSB = 1/2^16 °C
    int32_t raw_temp = (int32_t)((uint32_t)temp_data[2] << 16 |
                                 (uint32_t)temp_data[1] << 8  |
                                 (uint32_t)temp_data[0]);

    // Sign-extend from 24-bit
    if (raw_temp & 0x800000)
        raw_temp |= 0xFF000000;

    *temperature_c = raw_temp / 65536.0f;

    // Convert raw 24-bit pressure
    // BMP581 output: unsigned 24-bit, LSB = 1/2^6 Pa
    uint32_t raw_press = (uint32_t)press_data[2] << 16 |
                         (uint32_t)press_data[1] << 8  |
                         (uint32_t)press_data[0];

    *pressure_pa = raw_press / 64.0f;

    return true;
}

////////////////////////////////////////////////////////////
// PRESSURE TO ALTITUDE (Hypsometric formula)
////////////////////////////////////////////////////////////

float bmp581_pressure_to_altitude(float pressure_pa)
{
    // h = 44330 * (1 - (P / P0)^0.1903)
    return 44330.0f *
           (1.0f - powf(pressure_pa / SEA_LEVEL_PRESSURE,
                        0.1903f));
}
