#include "global.h"

#include "led_blinky.h"
#include "led_control.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"


// include task
#include "task_check_info.h"
#include "task_core_iot.h"
#include "task_toogle_boot.h"
#include "task_webserver.h"
#include "task_wifi.h"


void setup() {
  Serial.begin(115200);
  // Initialize LED PWM hardware
  check_info_File(0);
  init_led_pwm();

  xTaskCreate(task_wifi_manager, "Task WiFi", 4096, NULL, 5, NULL);
  xTaskCreate(task_read_sensor, "Task Read Sensor", 3072, NULL, 3, NULL);
  xTaskCreate(tiny_ml_task, "Tiny ML Task", 8192, NULL, 2, NULL);

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  xTaskCreate(task_lcd_display, "Task LCD Display", 3072, NULL, 2, NULL);

  xTaskCreate(task_webserver_run, "Task Webserver", 8192, NULL, 2, NULL);
  xTaskCreate(task_websocket_sender, "Task WS Sender", 4096, NULL, 2, NULL);

  xTaskCreate(task_core_iot_run, "Task CoreIOT", 8192, NULL, 2, NULL);
  xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop() {
  vTaskSuspend(NULL); 
}