#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bmp581.h"
#include "i2c_bus.h"
#include "lsm6dso.h"
#include "max30102.h"
#include "mmc56x3.h"
#include "tmp117.h"
#include "uart_comm.h"
#include "battery_monitor.h"
#include "display_oled.h"
#include "network_sos.h"

#include "driver/gpio.h"

#define GPIO_OUT_CRASH_ALERT 2
#define GPIO_IN_CRASH_CANCEL 3

////////////////////////////////////////////////////////////
// CRASH-DETECTION FLAGS
////////////////////////////////////////////////////////////

volatile int shock_flag = 0;
volatile int altitude_flag = 0;
volatile int stillness_flag = 0;
volatile int fall_confirmed_flag = 0;
volatile int mag_anomaly_flag = 0;
volatile int warning_flag = 0;

////////////////////////////////////////////////////////////
// ALTITUDE MONITORING CONFIG
////////////////////////////////////////////////////////////

#define ALTITUDE_DROP_THRESHOLD 2.0f // meters

////////////////////////////////////////////////////////////
// IMU TASK - shock/impact detection
////////////////////////////////////////////////////////////

void imu_task(void *arg) {
  lsm6dso_init();

  while (1) {
    float magnitude = lsm6dso_read_accel_magnitude();

    if (magnitude > 14.0f) {
      shock_flag = 1;
      printf("SEVERE IMPACT (%.2fg)\n", magnitude);
    } else if (magnitude > 8.0f) {
      warning_flag = 1;
      printf("WARNING IMPACT (%.2fg)\n", magnitude);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

////////////////////////////////////////////////////////////
// BAROMETER TASK - altitude drop detection
////////////////////////////////////////////////////////////

void barometer_task(void *arg) {
  if (!bmp581_init()) {
    printf("[BARO] Init failed - task stopped\n");
    vTaskDelete(NULL);
    return;
  }

  // Get baseline altitude
  float pressure, temperature;
  float baseline_alt = 0.0f;

  if (bmp581_read(&pressure, &temperature)) {
    baseline_alt = bmp581_pressure_to_altitude(pressure);

    printf("[BARO] Baseline: %.1f m "
           "(%.1f Pa, %.1f C)\n",
           baseline_alt, pressure, temperature);
  }

  while (1) {
    if (bmp581_read(&pressure, &temperature)) {
      float current_alt = bmp581_pressure_to_altitude(pressure);

      float drop = baseline_alt - current_alt;

      if (drop > ALTITUDE_DROP_THRESHOLD) {
        altitude_flag = 1;
        printf("[BARO] Altitude drop: "
               "%.1f m\n",
               drop);
      }

      // Update baseline slowly (rolling)
      baseline_alt = 0.99f * baseline_alt + 0.01f * current_alt;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

////////////////////////////////////////////////////////////
// MAGNETOMETER TASK - sensor fusion & anomaly detection
////////////////////////////////////////////////////////////

void magnetometer_task(void *arg) {
  if (!mmc56x3_init()) {
    printf("[MAG] Init failed - task stopped\n");
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    mag_data_t mag = mmc56x3_read();

    // Read accel for tilt compensation
    accel_data_t acc = lsm6dso_read_accel_xyz();

    orientation_t orient =
        mmc56x3_compute_orientation(mag, acc.x, acc.y, acc.z);

    // Check for sudden magnetic disturbance
    if (mmc56x3_detect_mag_anomaly(mag)) {
      mag_anomaly_flag = 1;
      printf("[MAG] Anomaly! heading=%.1f deg "
             "pitch=%.1f deg roll=%.1f deg\n",
             orient.heading, orient.pitch, orient.roll);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

////////////////////////////////////////////////////////////
// STILLNESS TASK - post-impact stillness check
////////////////////////////////////////////////////////////

void stillness_task(void *arg) {
  while (1) {
    if (shock_flag) {
      vTaskDelay(pdMS_TO_TICKS(5000));
      stillness_flag = 1;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

////////////////////////////////////////////////////////////
// SUPERVISOR TASK - fall confirmation
////////////////////////////////////////////////////////////

void supervisor_task(void *arg) {
  while (1) {
    if (warning_flag) {
      float temp = tmp117_read_temperature();

      printf("[SUPERVISOR] Sending warning alert...\n");
      uart_send_packet(1, 0, temp); // 1 = Warning

      warning_flag = 0;
    }

    if (shock_flag && stillness_flag) {
      fall_confirmed_flag = 1;

      float temp = tmp117_read_temperature();

      // Log if mag anomaly corroborates impact
      if (mag_anomaly_flag) {
        printf("[SUPERVISOR] Crash confirmed (IMU + MAG anomaly)\n");
      }

      printf("[SUPERVISOR] Alerting Secondary ESP32 on GPIO %d\n", GPIO_OUT_CRASH_ALERT);
      gpio_set_level(GPIO_OUT_CRASH_ALERT, 1);

      // 10 second countdown for cancellation
      bool cancelled = false;
      printf("[SUPERVISOR] Waiting 10 seconds for cancellation on GPIO %d...\n", GPIO_IN_CRASH_CANCEL);
      
      for (int i = 0; i < 100; i++) { // 100 * 100ms = 10s
        if (gpio_get_level(GPIO_IN_CRASH_CANCEL) == 1) {
          cancelled = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (cancelled) {
        printf("[SUPERVISOR] CRASH CANCELLED by Secondary ESP32! Aborting SOS.\n");
      } else {
        uart_send_packet(2, 0, temp); // 2 = Severe Crash
        
        // Trigger HTTP SOS POST to Backend Server
        printf("[SUPERVISOR] Triggering Wi-Fi SOS Alert!\n");
        network_send_sos_alert(true, battery_get_percentage(), temp);
      }

      // Reset alert pin and flags
      gpio_set_level(GPIO_OUT_CRASH_ALERT, 0);
      shock_flag = 0;
      stillness_flag = 0;
      mag_anomaly_flag = 0;
      fall_confirmed_flag = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

////////////////////////////////////////////////////////////
// PULSE TASK - heart rate + temperature monitoring
////////////////////////////////////////////////////////////

void pulse_task(void *arg) {
  max30102_init();
  tmp117_init();

  uint32_t samples[300];

  while (1) {
    for (int i = 0; i < 300; i++) {
      samples[i] = max30102_read_ir();
      vTaskDelay(pdMS_TO_TICKS(100));
    }

    int bpm = max30102_compute_bpm(samples, 300);

    float temp = tmp117_read_temperature();

    uart_send_packet(fall_confirmed_flag ? 2 : 0, bpm, temp);

    fall_confirmed_flag = 0;

    vTaskDelay(pdMS_TO_TICKS(600000));
  }
}

////////////////////////////////////////////////////////////
// DISPLAY & SYSTEM STATUS TASK
////////////////////////////////////////////////////////////

void display_task(void *arg) {
  while (1) {
    int batt = battery_get_percentage();
    
    // Check if any crash flags are active
    bool crash_status = (shock_flag || warning_flag || fall_confirmed_flag);
    
    // Check if Wi-Fi is successfully connected to the router/hotspot
    bool online_status = network_is_connected(); 

    display_update_status(online_status, crash_status, batt);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

////////////////////////////////////////////////////////////
// MAIN ENTRY POINT
////////////////////////////////////////////////////////////

void app_main(void) {
  printf("HELMET ENGINE STARTED\n");

  // Configure Crash Alert GPIOs
  gpio_config_t io_conf = {0};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_OUT_CRASH_ALERT);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
  gpio_set_level(GPIO_OUT_CRASH_ALERT, 0);

  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_IN_CRASH_CANCEL);
  io_conf.pull_down_en = 1; // Pulldown prevents floating false-positives
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // Initialize Wi-Fi in the background
  network_wifi_init();

  i2c_bus_init();
  uart_comm_init();
  battery_monitor_init();
  display_oled_init();

  xTaskCreate(imu_task, "IMU", 4096, NULL, 4, NULL);

  xTaskCreate(barometer_task, "BARO", 4096, NULL, 3, NULL);

  xTaskCreate(stillness_task, "STILLNESS", 4096, NULL, 2, NULL);

  xTaskCreate(supervisor_task, "SUPERVISOR", 4096, NULL, 2, NULL);

  xTaskCreate(magnetometer_task, "MAG", 4096, NULL, 3, NULL);

  xTaskCreate(pulse_task, "PULSE", 8192, NULL, 1, NULL);

  xTaskCreate(display_task, "DISPLAY", 4096, NULL, 1, NULL);
}
