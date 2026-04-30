#include "led_control.h"

// Define 4 GPIOs for 4 LEDs (You can change these to your actual pins)
const uint8_t LED_PINS[4] = {2, 4, 16, 17};
const uint8_t PWM_CHANNELS[4] = {0, 1, 2, 3};
const double PWM_FREQ = 5000;
const uint8_t PWM_RESOLUTION = 8;

void init_led_pwm() {
    for (int i = 0; i < 4; i++) {
        ledcSetup(PWM_CHANNELS[i], PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(LED_PINS[i], PWM_CHANNELS[i]);
        ledcWrite(PWM_CHANNELS[i], 0); // Start with LEDs OFF
    }
}

void led_set_state(uint8_t id, bool state) {
    if (id < 4) {
        ledcWrite(PWM_CHANNELS[id], state ? 255 : 0);
    }
}

void led_set_pwm(uint8_t id, uint8_t duty_cycle) {
    if (id < 4) {
        ledcWrite(PWM_CHANNELS[id], duty_cycle);
    }
}
