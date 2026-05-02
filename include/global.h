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

// FreeRTOS Handles
extern QueueHandle_t xQueueSensorToML;
extern QueueHandle_t xQueueMLData;
extern QueueHandle_t xQueueMLToServer;
extern QueueHandle_t xQueueMLToWeb;

extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif