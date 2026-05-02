#include "led_blinky.h"
#include "app_config.h"
#include "global.h"
#include "freertos/timers.h"

TimerHandle_t xLedTimer;
bool led_state = false;

// Timer callback to toggle LED
void led_timer_callback(TimerHandle_t xTimer) {
    led_state = !led_state;
    digitalWrite(LED_GPIO, led_state ? HIGH : LOW);
}

void led_blinky(void *pvParameters){
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LOW);
  
    // Create timer with default period of 2000ms (Normal)
    xLedTimer = xTimerCreate("LED Timer", pdMS_TO_TICKS(2000), pdTRUE, (void *)0, led_timer_callback);
    xTimerStart(xLedTimer, 0);

    ProcessedData received_data;
    TickType_t current_period = pdMS_TO_TICKS(2000);

    while(1) {                        
        if (xQueuePeek(xQueueMLData, &received_data, portMAX_DELAY) == pdTRUE) {
            TickType_t new_period;
            
            // Determine blink rate based on temperature status
            if (received_data.temperature < 30.0f) {
                new_period = pdMS_TO_TICKS(2000); // Normal (< 30°C)
            } else if (received_data.temperature <= 35.0f) {
                new_period = pdMS_TO_TICKS(500);  // Warning (30-35°C)
            } else {
                new_period = pdMS_TO_TICKS(100);  // Danger (> 35°C)
            }

            // Change timer period if needed
            if (current_period != new_period) {
                current_period = new_period;
                xTimerChangePeriod(xLedTimer, current_period, 0);
            }
        }
        // Prevent starvation, only check queue every 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}