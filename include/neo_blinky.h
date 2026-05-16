#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>



#define NEO_PIN 45
#define LED_COUNT 1 

void neo_blinky(void *pvParameters);
void neo_test_color(uint8_t r, uint8_t g, uint8_t b); // Diagnostic: hiện màu tùy chọn 3s


#endif