#include "tinyml.h"
#include "app_config.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

bool setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    if (input == nullptr || output == nullptr)
    {
        error_reporter->Report("Failed to get input or output tensor.");
        return false;
    }

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
    return true;
}

void tiny_ml_task(void *pvParameters)
{
    if (!setupTinyML())
    {
        Serial.println("TinyML setup failed. Deleting task.");
        vTaskDelete(NULL);
    }

    SensorData sensor_data;
    ProcessedData processed_data;

    while (1)
    {
        // Stage 1: Wait for raw data from the sensor task
        if (xQueueReceive(xQueueSensorToML, &sensor_data, portMAX_DELAY) == pdTRUE)
        {
            // If sensor data is invalid, pass it along without processing
            if (sensor_data.temperature <= -999.0f)
            {
                processed_data.temperature = sensor_data.temperature;
                processed_data.humidity = sensor_data.humidity;
                processed_data.anomaly_score = -1.0f;
            }
            else
            {
                // Stage 2: Run ML inference
                input->data.f[0] = sensor_data.temperature;
                input->data.f[1] = sensor_data.humidity;

                if (interpreter->Invoke() != kTfLiteOk)
                {
                    error_reporter->Report("Invoke failed");
                    continue; // Skip this data point
                }

                // Stage 3: Package the enriched data
                processed_data.temperature = sensor_data.temperature;
                processed_data.humidity = sensor_data.humidity;
                processed_data.anomaly_score = output->data.f[0];
                Serial.printf("Inference: Temp=%.1f, Humi=%.1f -> Score=%.4f\n", processed_data.temperature, processed_data.humidity, processed_data.anomaly_score);
            }

            // Stage 4: Fan-out the processed data to downstream tasks
            xQueueOverwrite(xQueueMLToDisplay, &processed_data);
            xQueueSend(xQueueMLToServer, &processed_data, (TickType_t)10);
            xQueueOverwrite(xQueueMLToWeb, &processed_data); // Gửi dữ liệu cho Web Server
        }
    }
}