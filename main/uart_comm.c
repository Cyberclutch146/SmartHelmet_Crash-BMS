#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "uart_comm.h"

////////////////////////////////////////////////////////////
// UART INIT
////////////////////////////////////////////////////////////

void uart_comm_init(void)
{
    uart_config_t cfg =
    {
        .baud_rate  = UART_COMM_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_COMM_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_COMM_PORT, &cfg);
    uart_set_pin(UART_COMM_PORT,
                 UART_COMM_TX,
                 UART_COMM_RX,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    printf("[UART] Initialized\n");
}

////////////////////////////////////////////////////////////
// SEND PACKET
////////////////////////////////////////////////////////////

void uart_send_packet(int fall, int bpm, float temp)
{
    char buffer[64];

    sprintf(buffer,
            "FALL:%d BPM:%d TEMP:%.1f\n",
            fall,
            bpm,
            temp);

    uart_write_bytes(UART_COMM_PORT,
                     buffer,
                     strlen(buffer));

    printf("[UART SENT] %s", buffer);
}
