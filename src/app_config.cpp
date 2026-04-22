#include "app_config.h"

// Global instance for application-wide configuration
AppConfig appConfig;

// --- Data Pipeline Queues ---
// These queues are used to pass data between different tasks.

// 1. From Sensor Task to TinyML Task
QueueHandle_t xQueueSensorToML = xQueueCreate(2, sizeof(SensorData));

// 2. From TinyML Task to Display Task
QueueHandle_t xQueueMLToDisplay = xQueueCreate(1, sizeof(ProcessedData));

// 3. From TinyML Task to Server Task (e.g., for MQTT)
QueueHandle_t xQueueMLToServer = xQueueCreate(5, sizeof(ProcessedData));

// 4. From TinyML Task to Web Server Task (for WebSocket updates)
QueueHandle_t xQueueMLToWeb = xQueueCreate(1, sizeof(ProcessedData));


// --- Semaphores & Mutexes ---

// Binary semaphore to signal when an internet connection is established
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();

// Mutex to protect access to the shared appConfig struct
SemaphoreHandle_t xConfigMutex = xSemaphoreCreateMutex();