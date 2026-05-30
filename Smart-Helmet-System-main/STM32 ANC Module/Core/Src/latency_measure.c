#include "latency_measure.h"
#include "stm32f4xx_hal.h"

// Core Debug registers
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)
#define DEMCR       (*(volatile uint32_t *)0xE000EDFC)
#define DEMCR_TRCENA    0x01000000
#define DWT_CTRL_CYCCNTENA 0x00000001

static uint32_t start_cycles = 0;

void Latency_Init(void) {
    // Enable TRCENA
    DEMCR |= DEMCR_TRCENA;
    // Reset cycle counter
    DWT_CYCCNT = 0;
    // Enable cycle counter
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

void Latency_Start(void) {
    start_cycles = DWT_CYCCNT;
}

uint32_t Latency_Stop(void) {
    uint32_t stop_cycles = DWT_CYCCNT;
    return (stop_cycles - start_cycles);
}
