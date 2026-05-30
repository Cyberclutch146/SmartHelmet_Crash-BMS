#include "bt_audio.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include <inttypes.h>

static const char *TAG = "BT_AUDIO";

// Your pipeline callback
static void (*audio_callback)(const uint8_t *data, uint32_t len) = NULL;

// 🔊 A2DP PCM audio callback (decoded audio)
static void a2dp_audio_cb(const uint8_t *data, uint32_t len)
{

    if (audio_callback)
    {
        audio_callback(data, len);
    }
}

// 📡 A2DP event handler (minimal)
static void a2dp_event_handler(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event)
    {

    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "A2DP connection state: %d",
                 param->conn_stat.state);
        break;

    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(TAG, "A2DP audio state: %d",
                 param->audio_stat.state);
        break;

    case ESP_A2D_AUDIO_CFG_EVT:
        ESP_LOGI(TAG, "A2DP audio config received");
        
        // Check if it's SBC and extract sample rate
        esp_a2d_cb_param_t *a2d = param;
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            uint32_t sample_rate = 44100;
            uint8_t samp_freq = a2d->audio_cfg.mcc.cie.sbc[0];
            
            // Extract from SBC Codec Specific Information Elements (CIE)
            // Bit 7: 16kHz, Bit 6: 32kHz, Bit 5: 44.1kHz, Bit 4: 48kHz
            if (samp_freq & 0x10) {
                sample_rate = 48000;
            } else if (samp_freq & 0x20) {
                sample_rate = 44100;
            }
            
            ESP_LOGI(TAG, "Negotiated sample rate: %lu Hz", sample_rate);
            
            // Notify STM32 via UART config packet
            extern void uart_stream_send_config(uint32_t sample_rate);
            uart_stream_send_config(sample_rate);
        }
        break;

    default:
        break;
    }
}

// 📡 GAP handler (for pairing, visibility)
static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {

    case ESP_BT_GAP_AUTH_CMPL_EVT:
        ESP_LOGI(TAG, "Authentication complete");
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
        ESP_LOGI(TAG, "PIN requested");
        esp_bt_pin_code_t pin_code;
        pin_code[0] = '1';
        pin_code[1] = '2';
        pin_code[2] = '3';
        pin_code[3] = '4';
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        break;

    default:
        break;
    }
}

// 🚀 Init function (called from main.c)
void bt_audio_init(void (*callback)(const uint8_t *data, uint32_t len))
{
    audio_callback = callback;

    ESP_LOGI(TAG, "Initializing Bluetooth...");

    // Release BLE memory (we only use classic BT)
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Init controller (ignore if already initialized)
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    // Enable controller (ignore if already enabled)
    ret = esp_bt_controller_enable(bt_cfg.mode);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Device name
    esp_bt_gap_set_device_name("ESP32_A2DP_TX");

    // GAP (pairing, visibility)
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_gap_cb));

    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code = {'1', '2', '3', '4'};
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    // A2DP sink
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2dp_event_handler));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2dp_audio_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_init());

    // Make device discoverable
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(
        ESP_BT_CONNECTABLE,
        ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(TAG, "Bluetooth initialized. Ready to pair.");
}