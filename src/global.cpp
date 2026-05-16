#include "global.h"

// Configuration variables
String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

boolean isWifiConnected = false;

// ─── Chế độ vận hành
// ────────────────────────────────────────────────────────────────
volatile bool g_is_auto_mode = true; // Mặc định: AUTO

// ─── Ngưỡng nghiệp vụ (Cloud)
// ────────────────────────────────────────────────────
volatile float g_anomaly_warn = 60.0f;
volatile float g_anomaly_danger = 80.0f;

// ─── Ngưỡng phần cứng cục bộ (Web Local)
// ──────────────────────────────────────────
volatile float g_led_thresh_warn = 30.0f;
volatile float g_led_thresh_danger = 35.0f;
volatile float g_neo_thresh_dry = 40.0f;
volatile float g_neo_thresh_wet = 70.0f;

// ─── Chu kỳ mặc định của các Task (ms) ───────────────────────────────────────
volatile uint32_t g_telemetry_interval   = 10000; // 10 giây
volatile uint32_t g_sensor_read_interval  = 1000;  // 1 giây đọc 10 lần
volatile uint32_t g_wifi_check_interval  = 10000; // 10 giây

// Mutex bảo vệ các biến ngưỡng và chu kỳ
SemaphoreHandle_t xMutexThresholds = xSemaphoreCreateMutex();

// ─── Data Pipeline Queues
// ────────────────────────────────────────────────────────────────
// 1. From Sensor Task to TinyML Task
QueueHandle_t xQueueSensorToML = xQueueCreate(2, sizeof(SensorData));

// 2. From TinyML Task to Shared Queue (LCD, LED, NeoPixel)
QueueHandle_t xQueueMLData = xQueueCreate(1, sizeof(ProcessedData));

// 3. From TinyML Task to Server Task
QueueHandle_t xQueueMLToServer = xQueueCreate(5, sizeof(ProcessedData));

// 4. From TinyML Task to Web Server Task
QueueHandle_t xQueueMLToWeb = xQueueCreate(1, sizeof(ProcessedData));

// Semaphore to signal when internet connection is available
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();