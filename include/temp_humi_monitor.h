#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"

void task_read_sensor(void *pvParameters);
void task_lcd_display(void *pvParameters);
#endif