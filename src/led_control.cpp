#include "led_control.h"

// Initialize the NeoPixel strip object
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
bool current_states[NUM_PIXELS] = {false, false, false, false};

void init_led_pwm() {
    pixels.begin();
    pixels.clear();
    pixels.show();
    Serial.printf("[LED] NeoPixel Initialized on Pin %d (4 LEDs)\n", NEOPIXEL_PIN);
}

void led_set_state(uint8_t id, bool state) {
    if (id < NUM_PIXELS) {
        current_states[id] = state;
        // Set only the Red channel as requested (R, G, B)
        if (state) {
            pixels.setPixelColor(id, pixels.Color(255, 0, 0));
        } else {
            pixels.setPixelColor(id, pixels.Color(0, 0, 0));
        }
        pixels.show();
    }
}

void led_set_pwm(uint8_t id, uint8_t duty_cycle) {
    if (id < NUM_PIXELS) {
        current_states[id] = (duty_cycle > 0);
        // Set Red channel with duty_cycle as brightness
        pixels.setPixelColor(id, pixels.Color(duty_cycle, 0, 0));
        pixels.show();
    }
}

bool led_get_state(uint8_t id) {
    if (id < NUM_PIXELS) {
        return current_states[id];
    }
    return false;
}
