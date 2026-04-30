#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

void init_led_pwm();
void led_set_state(uint8_t id, bool state);
void led_set_pwm(uint8_t id, uint8_t duty_cycle);

#endif
