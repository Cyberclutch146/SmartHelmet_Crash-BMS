#pragma once

#include <stdbool.h>

/**
 * Initialize the SSD1306 OLED Display over I2C.
 * Assumes the i2c_bus has already been initialized.
 */
void display_oled_init(void);

/**
 * Update the display with the current system status.
 * 
 * @param online True if connected/online
 * @param crash True if a crash was confirmed
 * @param battery_pct Current battery percentage 0-100
 */
void display_update_status(bool online, bool crash, int battery_pct);
