#include "app_config.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"
#include "coreiot.h"

// include task
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  check_info_File(0);

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);

  xTaskCreate(task_read_sensor, "Task Read Sensor", 3072, NULL, 3, NULL);
  xTaskCreate(task_lcd_display, "Task LCD Display", 3072, NULL, 2, NULL);

  xTaskCreate(task_webserver_run, "Task Webserver", 8192, NULL, 2, NULL);
  xTaskCreate(task_websocket_sender, "Task WS Sender", 4096, NULL, 2, NULL);

  xTaskCreate(tiny_ml_task, "Tiny ML Task", 8192, NULL, 2, NULL); // Needs more stack
  // xTaskCreate(coreiot_task, "CoreIOT Task", 4096, NULL, 2, NULL);
  xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  // With FreeRTOS, the loop() function is not needed.
  vTaskSuspend(NULL);
}