#include "global.h"

// Configuration variables
String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

boolean isWifiConnected = false;

// --- Data Pipeline Queues ---
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