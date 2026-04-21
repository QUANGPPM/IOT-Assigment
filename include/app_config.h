#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Struct to hold sensor data for passing through queues
struct SensorData {
    float temperature;
    float humidity;
};

// Struct to hold processed data after ML inference
struct ProcessedData {
    float temperature;
    float humidity;
    float anomaly_score;
    // You can add a const char* status if needed
};

// Struct for application-wide configuration
struct AppConfig {
    String WIFI_SSID;
    String WIFI_PASS;
    String CORE_IOT_TOKEN;
    String CORE_IOT_SERVER;
    String CORE_IOT_PORT;
};

// Extern declarations for global handles and config
extern QueueHandle_t xQueueSensorToML;
extern QueueHandle_t xQueueMLToDisplay;
extern QueueHandle_t xQueueMLToServer;
extern QueueHandle_t xQueueMLToWeb;

extern SemaphoreHandle_t xBinarySemaphoreInternet, xConfigMutex;
extern AppConfig appConfig;

#endif // APP_CONFIG_H