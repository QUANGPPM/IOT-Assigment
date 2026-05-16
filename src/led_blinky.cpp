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
            
            // Đọc ngưỡng nhiệt độ động trong vùng bảo vệ mutex
            float warn_thr, danger_thr;
            if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(10)) == pdTRUE) {
                warn_thr   = g_led_thresh_warn;
                danger_thr = g_led_thresh_danger;
                xSemaphoreGive(xMutexThresholds);
            } else {
                warn_thr   = 30.0f;
                danger_thr = 35.0f;
            }

            // Tốc độ nhấp nháy theo 3 mức nhiệt độ
            if (received_data.temperature < warn_thr) {
                new_period = pdMS_TO_TICKS(2000); // NORMAL
            } else if (received_data.temperature <= danger_thr) {
                new_period = pdMS_TO_TICKS(500);  // WARNING
            } else {
                new_period = pdMS_TO_TICKS(100);  // DANGER
            }

            // Thay đổi tốc độ hẹn giờ nếu cần
            if (current_period != new_period) {
                current_period = new_period;
                xTimerChangePeriod(xLedTimer, current_period, 0);
            }
        }
        // Nhưỡng CPU, chỉ kiểm tra mỗi 1 giây
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}