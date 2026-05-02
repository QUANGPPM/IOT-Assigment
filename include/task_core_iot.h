#ifndef __TASK_CORE_IOT_H__
#define __TASK_CORE_IOT_H__

#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "task_check_info.h"

void CORE_IOT_reconnect();
void task_core_iot_run(void *pvParameters);

#endif