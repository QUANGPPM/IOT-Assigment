#include "temp_humi_monitor.h"
#include "app_config.h"
#include "global.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(0x21, 16, 2); 

void task_read_sensor(void *pvParameters)
{
    Wire.begin(11, 12);
    if (!dht20.begin()) {
        Serial.println("[SENSOR] Error: DHT20 not found");
        vTaskDelete(NULL);
    }

    SensorData current_data;
    vTaskDelay(pdMS_TO_TICKS(50));
    
    while (1) {
        float temp_arr[10], humi_arr[10];
        int valid_samples = 0;

        for (int i = 0; i < 10; i++) {
            if (dht20.read() == DHT20_OK) {
                temp_arr[valid_samples] = dht20.getTemperature();
                humi_arr[valid_samples] = dht20.getHumidity();
                valid_samples++;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (valid_samples >= 3) {
            // Sorting and averaging logic (simplified view)
            for (int i = 0; i < valid_samples - 1; i++) {
                for (int j = i + 1; j < valid_samples; j++) {
                    if (temp_arr[i] > temp_arr[j]) { float t = temp_arr[i]; temp_arr[i] = temp_arr[j]; temp_arr[j] = t; }
                    if (humi_arr[i] > humi_arr[j]) { float h = humi_arr[i]; humi_arr[i] = humi_arr[j]; humi_arr[j] = h; }
                }
            }
            float sum_temp = 0, sum_humi = 0;
            for (int i = 1; i < valid_samples - 1; i++) { sum_temp += temp_arr[i]; sum_humi += humi_arr[i]; }
            current_data.temperature = sum_temp / (valid_samples - 2);
            current_data.humidity = sum_humi / (valid_samples - 2);
            Serial.printf("[SENSOR] T: %.1f C, H: %.1f %%\n", current_data.temperature, current_data.humidity);
        } else {
            current_data.temperature = -999.0f;
            current_data.humidity = -999.0f;
            Serial.println("[SENSOR] Error: Failed to read");
        }

        xQueueSend(xQueueSensorToML, &current_data, pdMS_TO_TICKS(100));
    }
}

void task_lcd_display(void *pvParameters)
{
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.print("LCD Ready");
    
    ProcessedData received_data;
    while (1) {
        if (xQueuePeek(xQueueMLData, &received_data, portMAX_DELAY) == pdTRUE) {
            lcd.clear();
            if (received_data.temperature <= -999.0f) {
                lcd.print("Sensor Error!");
            } else {
                lcd.setCursor(0, 0);
                lcd.printf("T:%.1fC H:%.1f%%", received_data.temperature, received_data.humidity);
                lcd.setCursor(0, 1);
                lcd.printf("S:%s", (received_data.status == STATUS_NORMAL) ? "NORMAL" : 
                                  (received_data.status == STATUS_WARNING) ? "WARNING" : "DANGER");
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}