#include "temp_humi_monitor.h"
#include "app_config.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(0x21, 16, 2); // I2C address 0x21, 16 chars, 2 lines

/**
 * @brief Task to read sensor data and push it to the ML pipeline.
 * This is the first stage (Producer).
 */
void task_read_sensor(void *pvParameters)
{
    // Initialize I2C for DHT20 sensor
    Wire.begin(11, 12);
    // Initialize DHT20 sensor
    if (!dht20.begin())
    {
        Serial.println("Failed to find DHT20 sensor! Deleting sensor task.");
        vTaskDelete(NULL);
    }

    SensorData current_data;
    vTaskDelay(pdMS_TO_TICKS(50));
    // Main loop
    while (1)
    {
        float temp_arr[10];
        float humi_arr[10];
        int valid_samples = 0;

        for (int i = 0; i < 10; i++) {
            int status = dht20.read();
            float t = dht20.getTemperature();
            float h = dht20.getHumidity();
            if (status == DHT20_OK && !isnan(t) && !isnan(h)) {
                temp_arr[valid_samples] = t;
                humi_arr[valid_samples] = h;
                valid_samples++;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (valid_samples >= 3) {
            // Sort arrays
            for (int i = 0; i < valid_samples - 1; i++) {
                for (int j = i + 1; j < valid_samples; j++) {
                    if (temp_arr[i] > temp_arr[j]) {
                        float t = temp_arr[i]; temp_arr[i] = temp_arr[j]; temp_arr[j] = t;
                    }
                    if (humi_arr[i] > humi_arr[j]) {
                        float h = humi_arr[i]; humi_arr[i] = humi_arr[j]; humi_arr[j] = h;
                    }
                }
            }
            // Average excluding min and max
            float sum_temp = 0, sum_humi = 0;
            for (int i = 1; i < valid_samples - 1; i++) {
                sum_temp += temp_arr[i];
                sum_humi += humi_arr[i];
            }
            current_data.temperature = sum_temp / (valid_samples - 2);
            current_data.humidity = sum_humi / (valid_samples - 2);
        } else {
            current_data.temperature = -999.0f;
            current_data.humidity = -999.0f;
            Serial.println("Failed to read from DHT sensor multiple times!");
        }

        // Print the results to Serial monitor for debugging
        Serial.printf("Sensor Read (Filtered): Temp=%.1f, Humi=%.1f\n", current_data.temperature, current_data.humidity);
        // Send the raw sensor data to the TinyML task for processing.
        // Use a small timeout to avoid blocking indefinitely if the queue is full.
        if (xQueueSend(xQueueSensorToML, &current_data, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("TinyML queue is full. Data was not sent.");
        }
    }
}

/**
 * @brief Task to display processed data on the LCD.
 * This is a final stage consumer.
 */
void task_lcd_display(void *pvParameters)
{
    // Initialize LCD
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.print("Initializing...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    ProcessedData received_data;

    while (1)
    {
        // Use xQueuePeek to read the latest data without removing it
        if (xQueuePeek(xQueueMLData, &received_data, portMAX_DELAY) == pdTRUE)
        {
            lcd.clear();
            if (received_data.temperature <= -999.0f)
            {
                lcd.setCursor(0, 0);
                lcd.print("Sensor Error!");
            }
            else
            {
                const char *status_str = "NORMAL";
                if (received_data.status == STATUS_WARNING) status_str = "WARNING";
                else if (received_data.status == STATUS_DANGER) status_str = "DANGER";

                lcd.setCursor(0, 0);
                lcd.printf("T:%.1fC H:%.1f%%", received_data.temperature, received_data.humidity);
                lcd.setCursor(0, 1);
                lcd.printf("STATUS: %s", status_str);
            }
            // Update LCD periodically, no need to spam the I2C bus
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}