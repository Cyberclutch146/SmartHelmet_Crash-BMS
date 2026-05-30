#include <string.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"

#include "bt_audio.h"
#include "uart_stream.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "MAIN";

typedef struct
{
    uint32_t len;
    uint8_t data[4096]; // must fit full A2DP packet (~3584 bytes)
} audio_packet_t;

static QueueHandle_t audio_queue;

static void audio_pipeline_callback(const uint8_t *data, uint32_t len)
{
    // Allocate packet dynamically (avoid large stack usage in BT thread)
    audio_packet_t *pkt = malloc(sizeof(audio_packet_t));
    if (!pkt)
    {
        return; // drop if allocation fails
    }

    if (len > sizeof(pkt->data))
    {
        len = sizeof(pkt->data);
    }

    pkt->len = len;
    memcpy(pkt->data, data, len);

    // Non-blocking send
    if (xQueueSend(audio_queue, &pkt, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        free(pkt); // free if queue full
    }
}

static void audio_task(void *arg)
{
    audio_packet_t *pkt;

    while (1)
    {
        if (xQueueReceive(audio_queue, &pkt, portMAX_DELAY))
        {
            // Convert bytes → int16 samples
            int16_t *pcm = (int16_t *)pkt->data;
            int total_samples = pkt->len / 2; // int16 samples (L, R interleaved)

            // Send RAW 44.1kHz stereo with carry buffer
            #define FRAME_SAMPLES 256

            static int16_t carry_buffer[FRAME_SAMPLES];
            static int carry_count = 0;

            int i = 0;

            // Fill carry buffer first if needed
            if (carry_count > 0)
            {
                int needed = FRAME_SAMPLES - carry_count;
                int to_copy = (total_samples < needed) ? total_samples : needed;

                memcpy(&carry_buffer[carry_count], pcm, to_copy * sizeof(int16_t));
                carry_count += to_copy;
                i += to_copy;

                if (carry_count == FRAME_SAMPLES)
                {
                    uart_stream_send(carry_buffer, FRAME_SAMPLES);
                    carry_count = 0;
                }
            }

            // Send full frames directly
            while (i + FRAME_SAMPLES <= total_samples)
            {
                uart_stream_send(&pcm[i], FRAME_SAMPLES);
                i += FRAME_SAMPLES;
            }

            // Store leftovers for next iteration
            if (i < total_samples)
            {
                carry_count = total_samples - i;
                memcpy(carry_buffer, &pcm[i], carry_count * sizeof(int16_t));
            }
            free(pkt);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 A2DP → UART transmitter");

    // 🔧 NVS (required for Bluetooth)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 🔧 UART init
    ESP_LOGI(TAG, "Initializing UART...");
    uart_stream_init();


    // 🔧 Create audio queue + task
    audio_queue = xQueueCreate(20, sizeof(audio_packet_t *));
    xTaskCreate(audio_task, "audio_task", 8192, NULL, 5, NULL);

    // 🔧 Bluetooth A2DP init
    ESP_LOGI(TAG, "Initializing Bluetooth A2DP...");
    bt_audio_init(audio_pipeline_callback);

    ESP_LOGI(TAG, "System ready. Waiting for Bluetooth audio...");
}