#include "battery_monitor.h"
#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

// Hardware Configuration - CHANGE THESE IF NEEDED
#define BATT_ADC_UNIT      ADC_UNIT_1
#define BATT_ADC_CHANNEL   ADC_CHANNEL_3 // GPIO 4 on ESP32-S3
#define BATT_ADC_ATTEN     ADC_ATTEN_DB_12 // 0 - ~3.1V

// Voltage divider multiplier. 
// E.g. for a 30k and 10k divider, V_batt = V_adc * (30+10)/10 = 4.0
#define VOLTAGE_DIVIDER_RATIO 4.0f

// 2S Li-ion battery voltage limits
#define BATT_MAX_VOLTAGE 8.4f
#define BATT_MIN_VOLTAGE 6.4f

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool cali_enabled = false;

static const char *TAG = "BATT_MON";

bool battery_monitor_init(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = BATT_ADC_UNIT,
    };
    if (adc_oneshot_new_unit(&init_config1, &adc1_handle) != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed");
        return false;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = BATT_ADC_ATTEN,
    };
    if (adc_oneshot_config_channel(adc1_handle, BATT_ADC_CHANNEL, &config) != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel config failed");
        return false;
    }

    // Attempt to initialize calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATT_ADC_UNIT,
        .chan = BATT_ADC_CHANNEL,
        .atten = BATT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle) == ESP_OK) {
        cali_enabled = true;
        ESP_LOGI(TAG, "ADC Calibration enabled");
    } else {
        ESP_LOGW(TAG, "ADC Calibration failed (not supported on this board?)");
    }

    return true;
}

float battery_get_voltage(void) {
    if (!adc1_handle) return 0.0f;

    int raw_val = 0;
    adc_oneshot_read(adc1_handle, BATT_ADC_CHANNEL, &raw_val);

    float adc_voltage_mv = 0;
    if (cali_enabled) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(adc1_cali_handle, raw_val, &voltage_mv);
        adc_voltage_mv = (float)voltage_mv;
    } else {
        // Fallback rough estimate if calibration is missing (3.1V max for 12 attenuation, 12-bit ADC)
        adc_voltage_mv = ((float)raw_val / 4095.0f) * 3100.0f;
    }

    // Convert from mV to V, and apply voltage divider ratio
    float battery_v = (adc_voltage_mv / 1000.0f) * VOLTAGE_DIVIDER_RATIO;
    return battery_v;
}

int battery_get_percentage(void) {
    float voltage = battery_get_voltage();
    
    // Clamp voltage to expected range
    if (voltage > BATT_MAX_VOLTAGE) voltage = BATT_MAX_VOLTAGE;
    if (voltage < BATT_MIN_VOLTAGE) voltage = BATT_MIN_VOLTAGE;

    // Linear mapping from 6.4V-8.4V to 0-100%
    float pct = ((voltage - BATT_MIN_VOLTAGE) / (BATT_MAX_VOLTAGE - BATT_MIN_VOLTAGE)) * 100.0f;
    
    int percentage = (int)pct;
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    return percentage;
}
