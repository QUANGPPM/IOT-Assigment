#ifndef __LED_CONTROL_H__
#define __LED_CONTROL_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Change this to the pin where you plugged your OhStem 4 LED RGB Module
// For Yolo Uno Grove ports: D1 is 16, D2 is 4, D3 is 14, D4 is 32
#define NEOPIXEL_PIN 36 
#define NUM_PIXELS   4

void init_led_pwm(); 
void led_set_state(uint8_t id, bool state);
void led_set_pwm(uint8_t id, uint8_t duty_cycle);
bool led_get_state(uint8_t id);

#endif
