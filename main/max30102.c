#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "max30102.h"

////////////////////////////////////////////////////////////
// MAX30102 I2C ADDRESS & REGISTERS
////////////////////////////////////////////////////////////

#define MAX30102_ADDR       0x57

#define REG_FIFO_DATA       0x07
#define REG_FIFO_CONFIG     0x08
#define REG_MODE_CONFIG     0x09
#define REG_SPO2_CONFIG     0x0A
#define REG_LED1_PA         0x0C
#define REG_LED2_PA         0x0D

////////////////////////////////////////////////////////////
// VALIDATION
////////////////////////////////////////////////////////////

#define BPM_MIN_VALID       75
#define BPM_MAX_VALID       130
#define IR_THRESHOLD        70000

////////////////////////////////////////////////////////////
// INIT
////////////////////////////////////////////////////////////

void max30102_init(void)
{
    // Reset
    i2c_write_reg(MAX30102_ADDR,
                  REG_MODE_CONFIG,
                  0x40);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Configure
    i2c_write_reg(MAX30102_ADDR,
                  REG_FIFO_CONFIG,
                  0x0F);

    i2c_write_reg(MAX30102_ADDR,
                  REG_SPO2_CONFIG,
                  0x27);

    i2c_write_reg(MAX30102_ADDR,
                  REG_LED1_PA,
                  0x24);

    i2c_write_reg(MAX30102_ADDR,
                  REG_LED2_PA,
                  0x24);

    // SpO2 mode
    i2c_write_reg(MAX30102_ADDR,
                  REG_MODE_CONFIG,
                  0x03);

    printf("[PULSE] MAX30102 ready\n");
}

////////////////////////////////////////////////////////////
// READ SINGLE IR SAMPLE
////////////////////////////////////////////////////////////

uint32_t max30102_read_ir(void)
{
    uint8_t data[3];

    i2c_read_bytes(MAX30102_ADDR,
                   REG_FIFO_DATA,
                   data,
                   3);

    return ((uint32_t)data[0] << 16) |
           ((uint32_t)data[1] << 8)  |
            (uint32_t)data[2];
}

////////////////////////////////////////////////////////////
// COMPUTE BPM FROM IR SAMPLES (peak detection)
////////////////////////////////////////////////////////////

int max30102_compute_bpm(uint32_t *samples, int count)
{
    int peaks = 0;

    for (int i = 2; i < count - 2; i++)
    {
        if (samples[i] > samples[i - 1] &&
            samples[i] > samples[i + 1] &&
            samples[i] > samples[i - 2] &&
            samples[i] > samples[i + 2] &&
            samples[i] > IR_THRESHOLD)
        {
            peaks++;
        }
    }

    int bpm = peaks * 2;

    if (bpm >= BPM_MIN_VALID && bpm <= BPM_MAX_VALID)
        return bpm;

    return -1;
}
