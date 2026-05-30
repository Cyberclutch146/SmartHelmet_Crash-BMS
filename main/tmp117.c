#include <stdio.h>
#include "driver/i2c.h"
#include "i2c_bus.h"
#include "tmp117.h"

////////////////////////////////////////////////////////////
// TMP117 I2C ADDRESS & REGISTERS
////////////////////////////////////////////////////////////

#define TMP117_ADDR         0x48

#define TMP117_REG_TEMP     0x00
#define TMP117_REG_CONFIG   0x01

////////////////////////////////////////////////////////////
// VALIDATION
////////////////////////////////////////////////////////////

#define TEMP_MIN_VALID      35.0f
#define TEMP_MAX_VALID      38.0f

////////////////////////////////////////////////////////////
// INIT
////////////////////////////////////////////////////////////

void tmp117_init(void)
{
    // Config: continuous conversion, 1 Hz,
    // no averaging
    // config_value = 0x00A0
    uint8_t config_data[3];

    config_data[0] = TMP117_REG_CONFIG;
    config_data[1] = 0x00;  // MSB
    config_data[2] = 0xA0;  // LSB

    i2c_master_write_to_device(I2C_BUS_PORT,
                               TMP117_ADDR,
                               config_data,
                               3,
                               100);

    printf("[TEMP] TMP117 initialized\n");
}

////////////////////////////////////////////////////////////
// READ TEMPERATURE (°C)
////////////////////////////////////////////////////////////

float tmp117_read_temperature(void)
{
    uint8_t data[2];

    i2c_read_bytes(TMP117_ADDR,
                   TMP117_REG_TEMP,
                   data,
                   2);

    int16_t raw = (data[0] << 8) | data[1];

    return raw * 0.0078125f;
}
