#include "display_oled.h"
#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define OLED_ADDR 0x3C

// A compact subset of 5x7 font for ASCII 32 to 90 (Space, numbers, uppercase letters)
// This saves space in the source file while providing everything we need for the status screen.
static const uint8_t font5x7[59][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
    {0x00, 0x00, 0x2f, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // 35 #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // 41 )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 48 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // 64 @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 65 A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 68 D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 71 G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 72 H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 74 J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 77 M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 78 N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 79 O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 81 Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 84 T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 85 U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 86 V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // 90 Z
};

static void oled_send_cmd(uint8_t cmd) {
    i2c_write_reg(OLED_ADDR, 0x00, cmd);
}

static void oled_send_data(uint8_t data) {
    i2c_write_reg(OLED_ADDR, 0x40, data);
}

static void oled_set_cursor(uint8_t col, uint8_t page) {
    oled_send_cmd(0xB0 | (page & 0x07));
    oled_send_cmd(0x00 | (col & 0x0F));
    oled_send_cmd(0x10 | ((col >> 4) & 0x0F));
}

static void oled_clear(void) {
    for (int page = 0; page < 8; page++) {
        oled_set_cursor(0, page);
        for (int col = 0; col < 128; col++) {
            oled_send_data(0x00);
        }
    }
}

static void oled_print_char(char c) {
    if (c < 32 || c > 90) c = 32; // Map outside chars to space
    int idx = c - 32;
    for (int i = 0; i < 5; i++) {
        oled_send_data(font5x7[idx][i]);
    }
    oled_send_data(0x00); // letter spacing
}

static void oled_print_string(const char *str, uint8_t col, uint8_t page) {
    oled_set_cursor(col, page);
    while (*str) {
        // Convert lowercase to uppercase to fit our limited font array
        char c = *str;
        if (c >= 'a' && c <= 'z') c -= 32;
        oled_print_char(c);
        str++;
    }
}

static uint16_t scale_byte_2x(uint8_t b) {
    uint16_t res = 0;
    for (int i=0; i<8; i++) {
        if (b & (1<<i)) {
            res |= (3 << (i*2));
        }
    }
    return res;
}

static void oled_print_char_2x(char c, uint8_t col, uint8_t page) {
    if (c < 32 || c > 90) c = 32;
    int idx = c - 32;
    
    // Top half
    oled_set_cursor(col, page);
    for (int i = 0; i < 5; i++) {
        uint16_t scaled = scale_byte_2x(font5x7[idx][i]);
        uint8_t top = scaled & 0xFF;
        oled_send_data(top);
        oled_send_data(top);
    }
    oled_send_data(0); oled_send_data(0);
    
    // Bottom half
    oled_set_cursor(col, page + 1);
    for (int i = 0; i < 5; i++) {
        uint16_t scaled = scale_byte_2x(font5x7[idx][i]);
        uint8_t bot = (scaled >> 8) & 0xFF;
        oled_send_data(bot);
        oled_send_data(bot);
    }
    oled_send_data(0); oled_send_data(0);
}

static void oled_print_string_2x(const char *str, uint8_t col, uint8_t page) {
    while (*str) {
        char c = *str;
        if (c >= 'a' && c <= 'z') c -= 32;
        oled_print_char_2x(c, col, page);
        col += 12; // 5*2 width + 2 spacing
        str++;
    }
}

void display_oled_init(void) {
    vTaskDelay(pdMS_TO_TICKS(100)); // wait for oled to boot

    // Standard SSD1306 Initialization Sequence
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x20); // Set Memory Addressing Mode
    oled_send_cmd(0x02); // Page Addressing Mode
    oled_send_cmd(0x81); // Set Contrast Control
    oled_send_cmd(0xFF); // Max contrast
    oled_send_cmd(0xA1); // Set Segment Re-map
    oled_send_cmd(0xC8); // Set COM Output Scan Direction
    oled_send_cmd(0xA6); // Set Normal Display
    oled_send_cmd(0x8D); // Set Charge Pump
    oled_send_cmd(0x14); // Enable Charge Pump
    oled_send_cmd(0xAF); // Display ON

    oled_clear();
}

void display_update_status(bool online, bool crash, int battery_pct) {
    char buf[32];

    // Status on top line (normal 1x size)
    sprintf(buf, "STATUS: %s", online ? "ONLINE " : "OFFLINE");
    oled_print_string(buf, 0, 0);

    // Crash flag (Double size, starts at page 2, spans page 2 and 3)
    sprintf(buf, "CRASH:%s", crash ? "YES" : "NO ");
    oled_print_string_2x(buf, 0, 2);

    // Battery percentage (Double size, starts at page 4, spans page 4 and 5)
    sprintf(buf, "BATT:%d%% ", battery_pct);
    oled_print_string_2x(buf, 0, 4);
}
