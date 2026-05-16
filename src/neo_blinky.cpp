#include "neo_blinky.h"
#include "app_config.h"
#include "global.h"

// Strip khai báo static để có thể dùng từ cả neo_blinky task lẫn neo_test_color()
static Adafruit_NeoPixel s_strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// ─── Diagnostic: hiện màu tùy chọn trực tiếp (gọi từ task_handler) ──────────
void neo_test_color(uint8_t r, uint8_t g, uint8_t b) {
    s_strip.setPixelColor(0, s_strip.Color(r, g, b));
    s_strip.show();
}

// ─── Task chính: phản ánh độ ẩm qua màu sắc theo ngưỡng động ─────────────────
void neo_blinky(void *pvParameters) {
    s_strip.begin();
    s_strip.clear();
    s_strip.show();

    ProcessedData received_data;

    while (1) {
        if (xQueuePeek(xQueueMLData, &received_data, portMAX_DELAY) == pdTRUE) {
            // Đọc ngưỡng độ ẩm động trong vùng bảo vệ mutex
            float dry_thr, wet_thr;
            if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(10)) == pdTRUE) {
                dry_thr = g_neo_thresh_dry;
                wet_thr = g_neo_thresh_wet;
                xSemaphoreGive(xMutexThresholds);
            } else {
                dry_thr = 40.0f;
                wet_thr = 70.0f;
            }

            uint32_t color;
            // 3 mức độ ẩm: Khô / Tối ưu / Ướt
            if (received_data.humidity < dry_thr) {
                color = s_strip.Color(255, 255, 0); // Vàng  — Khô
            } else if (received_data.humidity <= wet_thr) {
                color = s_strip.Color(0, 255, 0);   // Xanh lá — Tối ưu
            } else {
                color = s_strip.Color(0, 0, 255);   // Xanh dương — Ướt
            }

            s_strip.setPixelColor(0, color);
            s_strip.show();
        }

        // Nhường CPU
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}