#pragma once
#include <stdint.h>

void bt_audio_init(void (*callback)(const uint8_t *data, uint32_t len));