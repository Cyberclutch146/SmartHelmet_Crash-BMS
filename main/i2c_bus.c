#include <stdio.h>
#include "driver/i2c.h"
#include "i2c_bus.h"

////////////////////////////////////////////////////////////
// I2C BUS INIT
////////////////////////////////////////////////////////////

SemaphoreHandle_t i2c_mutex = NULL;

void i2c_bus_init(void)
{
    i2c_config_t conf =
    {
        .mode           = I2C_MODE_MASTER,
        .sda_io_num     = I2C_BUS_SDA,
        .scl_io_num     = I2C_BUS_SCL,
        .sda_pullup_en  = GPIO_PULLUP_ENABLE,
        .scl_pullup_en  = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_BUS_FREQ_HZ
    };

    i2c_param_config(I2C_BUS_PORT, &conf);
    i2c_driver_install(I2C_BUS_PORT, conf.mode, 0, 0, 0);

    i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        printf("[I2C] Failed to create mutex\n");
    }

    printf("[I2C] Bus initialized\n");
}

////////////////////////////////////////////////////////////
// WRITE SINGLE REGISTER
////////////////////////////////////////////////////////////

void i2c_write_reg(uint8_t dev_addr,
                   uint8_t reg,
                   uint8_t val)
{
    if (i2c_mutex && xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t data[2] = {reg, val};
        i2c_master_write_to_device(I2C_BUS_PORT,
                                   dev_addr,
                                   data,
                                   2,
                                   100);
        xSemaphoreGive(i2c_mutex);
    }
}

////////////////////////////////////////////////////////////
// READ MULTIPLE BYTES
////////////////////////////////////////////////////////////

void i2c_read_bytes(uint8_t dev_addr,
                    uint8_t reg,
                    uint8_t *data,
                    int len)
{
    if (i2c_mutex && xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
        i2c_master_write_read_device(I2C_BUS_PORT,
                                    dev_addr,
                                    &reg,
                                    1,
                                    data,
                                    len,
                                    100);
        xSemaphoreGive(i2c_mutex);
    }
}
