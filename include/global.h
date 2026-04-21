#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// These global configuration variables are still used by non-refactored tasks.
// They should be moved into the AppConfig struct and protected by a mutex in a future step.
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;

extern QueueHandle_t xQueueSensorToML;;
extern QueueHandle_t xQueueMLToDisplay;
extern QueueHandle_t xQueueMLToServer;
extern QueueHandle_t xQueueMLToWeb;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
#endif