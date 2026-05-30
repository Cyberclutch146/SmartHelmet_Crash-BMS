idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "uart_comm.c"
        "lsm6dso.c"
        "bmp581.c"
        "max30102.c"
        "tmp117.c"
        "mmc56x3.c"
        "battery_monitor.c"
        "display_oled.c"
        "network_sos.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_gpio esp_driver_uart log esp_timer esp_adc esp_wifi esp_event nvs_flash esp_http_client esp_netif
)