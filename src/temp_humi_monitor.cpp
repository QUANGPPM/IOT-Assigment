#include "temp_humi_monitor.h"


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
        // Read data from DHT20 sensor
        int status = dht20.read();
        current_data.temperature = dht20.getTemperature();
        current_data.humidity = dht20.getHumidity();

        // Check if any reads failed and exit early
        if (status != DHT20_OK || isnan(current_data.temperature) || isnan(current_data.humidity))
        {
            Serial.println("Failed to read from DHT sensor!");
            // Set invalid values to signal an error downstream
            current_data.temperature = -999.0f;
            current_data.humidity = -999.0f;
        }
        // Print the results to Serial monitor for debugging
        Serial.printf("Sensor Read: Temp=%.1f, Humi=%.1f\n", current_data.temperature, current_data.humidity);
        // Send the raw sensor data to the TinyML task for processing.
        // Use a small timeout to avoid blocking indefinitely if the queue is full.
        if (xQueueSend(xQueueSensorToML, &current_data, pdMS_TO_TICKS(100)) != pdPASS)
        {
            Serial.println("TinyML queue is full. Data was not sent.");
        }

        // Wait for 5 seconds before the next reading
        vTaskDelay(pdMS_TO_TICKS(3000));
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
        // Wait indefinitely for processed data from the TinyML task
        if (xQueueReceive(xQueueMLToDisplay, &received_data, portMAX_DELAY) == pdTRUE)
        {
            lcd.clear();
            if (received_data.temperature <= -999.0f)
            {
                lcd.setCursor(0, 0);
                lcd.print("Sensor Error!");
            }
            else
            {
                // Example: Display "ANOMALY" if score is high
                const char *status = (received_data.anomaly_score > 0.7) ? "ANOMALY" : "NORMAL";

                lcd.setCursor(0, 0);
                lcd.printf("T:%.1fC H:%.1f%%", received_data.temperature, received_data.humidity);
                lcd.setCursor(0, 1);
                lcd.printf("Status: %s", status);
            }
        }
    }
}