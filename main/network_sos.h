#pragma once

#include <stdbool.h>

/**
 * @brief Initialize NVS, WiFi station, and connect to the access point.
 * This function blocks until connection is established or a timeout occurs.
 */
void network_wifi_init(void);

/**
 * @brief Check if the WiFi is currently connected.
 * @return true if connected, false otherwise
 */
bool network_is_connected(void);

/**
 * @brief Send an SOS alert via HTTP POST to the backend server.
 * 
 * @param crash True if a severe crash is detected, False for a warning.
 * @param battery_pct The current battery percentage to include in the payload.
 * @param temp The current temperature to include in the payload.
 */
void network_send_sos_alert(bool crash, int battery_pct, float temp);
