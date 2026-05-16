#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "app_config.h"

// Configuration variables (populated from info.dat)
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;

// ─── Chế độ vận hành (AUTO / MANUAL) ────────────────────────────────────────
// Được ghi bởi task_core_iot (Shared Attributes), đọc bởi các task khác
extern volatile bool g_is_auto_mode; // true = AUTO, false = MANUAL

// ─── Ngưỡng nghiệp vụ (đồng bộ từ Cloud qua Shared Attributes) ──────────────
extern volatile float g_anomaly_warn;    // Mặc định: 60.0
extern volatile float g_anomaly_danger;  // Mặc định: 80.0

// ─── Ngưỡng phần cứng cục bộ (cấu hình qua Web Local) ───────────────────────
extern volatile float g_led_thresh_warn;   // Nhiệt độ bắt đầu Warning (°C), mặc định 30.0
extern volatile float g_led_thresh_danger; // Nhiệt độ Danger (°C), mặc định 35.0
extern volatile float g_neo_thresh_dry;    // Độ ẩm thấp - Khô (%), mặc định 40.0
extern volatile float g_neo_thresh_wet;    // Độ ẩm cao  - Ướt (%), mặc định 70.0

// ─── Chu kỳ các Task (ms) ────────────────────────────────────────────────────
extern volatile uint32_t g_telemetry_interval;   // Chu kỳ gửi telemetry (mặc định 10s)
extern volatile uint32_t g_sensor_read_interval;  // Chu kỳ đọc cảm biến (mặc định 5s)
extern volatile uint32_t g_wifi_check_interval;  // Chu kỳ kiểm tra WiFi (mặc định 10s)

// Mutex bảo vệ tất cả biến ngưỡng khi đọc/ghi từ nhiều task
extern SemaphoreHandle_t xMutexThresholds;

// FreeRTOS Handles
extern QueueHandle_t xQueueSensorToML;
extern QueueHandle_t xQueueMLData;
extern QueueHandle_t xQueueMLToServer;
extern QueueHandle_t xQueueMLToWeb;

extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif