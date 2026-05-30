#pragma once

#include <stdint.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t i2c_mutex;

////////////////////////////////////////////////////////////
// I2C BUS CONFIGURATION
////////////////////////////////////////////////////////////

#define I2C_BUS_SDA         5
#define I2C_BUS_SCL         6
#define I2C_BUS_PORT        I2C_NUM_0
#define I2C_BUS_FREQ_HZ     400000

////////////////////////////////////////////////////////////
// PUBLIC API
////////////////////////////////////////////////////////////

void i2c_bus_init(void);

void i2c_write_reg(uint8_t dev_addr,
                   uint8_t reg,
                   uint8_t val);

void i2c_read_bytes(uint8_t dev_addr,
                    uint8_t reg,
                    uint8_t *data,
                    int len);
