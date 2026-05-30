#pragma once

#include <stdbool.h>

/**
 * Initialize the battery monitor subsystem.
 * This sets up the ADC for reading the voltage divider.
 * 
 * @return true if successful, false otherwise
 */
bool battery_monitor_init(void);

/**
 * Get the current estimated battery percentage (0-100%).
 * Uses a mapping from 6.4V to 8.4V for a 2S Li-ion pack.
 * 
 * @return integer percentage 0-100
 */
int battery_get_percentage(void);

/**
 * Get the raw battery voltage.
 * 
 * @return float representing true battery voltage
 */
float battery_get_voltage(void);
