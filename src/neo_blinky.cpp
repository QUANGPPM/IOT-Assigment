#include "neo_blinky.h"
#include "app_config.h"
#include "global.h"

void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();

    ProcessedData received_data;

    while(1) {                          
        if (xQueuePeek(xQueueMLData, &received_data, portMAX_DELAY) == pdTRUE) {
            uint32_t color;
            
            if (received_data.humidity < 40.0f) {
                color = strip.Color(255, 255, 0); // Yellow (Dry)
            } else if (received_data.humidity <= 70.0f) {
                color = strip.Color(0, 255, 0);   // Green (Optimal)
            } else {
                color = strip.Color(0, 0, 255);   // Blue (Wet)
            }

            strip.setPixelColor(0, color);
            strip.show();
        }

        // Prevent starvation
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}