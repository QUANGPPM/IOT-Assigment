#include "task_core_iot.h"
#include "led_control.h"
#include "app_config.h"
#include "freertos/timers.h"

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

constexpr char LED_STATE_ATTR[] = "ledState";
constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";

volatile int ledMode = 0;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

constexpr int16_t telemetrySendInterval = 10000U;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR,
    BLINKING_INTERVAL_ATTR
};

TimerHandle_t xLed1Timer = NULL;
bool led1_current_state = false;

void led1_timer_callback(TimerHandle_t xTimer) {
    if (ledState) {
        led1_current_state = !led1_current_state;
        led_set_state(1, led1_current_state);
    }
}

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0)
        {
            ledState = it->value().as<bool>();
            if (ledState) {
                if (xLed1Timer != NULL) xTimerStart(xLed1Timer, 0);
            } else {
                if (xLed1Timer != NULL) xTimerStop(xLed1Timer, 0);
                led_set_state(1, false);
            }
            Serial.print("LED 1 state is set to (Shared Attribute): ");
            Serial.println(ledState);
        }
        else if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
        {
            const uint16_t new_interval = it->value().as<uint16_t>();
            if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
            {
                blinkingInterval = new_interval;
                if (xLed1Timer != NULL) {
                    xTimerChangePeriod(xLed1Timer, pdMS_TO_TICKS(blinkingInterval), 0);
                }
                Serial.print("Blinking interval is set to: ");
                Serial.println(new_interval);
            }
        }
    }
}

RPC_Response setLED(const RPC_Data &data)
{
    Serial.println("Received Switch state");
    bool newState = data;
    Serial.print("Switch state change (RPC for LED 2 & 3): ");
    Serial.println(newState);
    
    led_set_state(2, newState);
    led_set_state(3, newState);
    
    return RPC_Response("setLED", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
    RPC_Callback{"setLED", setLED}};

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void CORE_IOT_sendata(String mode, String feed, String data)
{
    if (mode == "attribute")
    {
        tb.sendAttributeData(feed.c_str(), data);
    }
    else if (mode == "telemetry")
    {
        float value = data.toFloat();
        tb.sendTelemetryData(feed.c_str(), value);
    }
    else
    {
        // handle unknown mode
    }
}

void CORE_IOT_reconnect()
{
    if (!tb.connected())
    {
        if (!tb.connect(CORE_IOT_SERVER.c_str(), CORE_IOT_TOKEN.c_str(), CORE_IOT_PORT.toInt()))
        {
            // Serial.println("Failed to connect");
            return;
        }

        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        Serial.println("Subscribing for RPC...");
        if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        {
            // Serial.println("Failed to subscribe for RPC");
            return;
        }

        if (!tb.Shared_Attributes_Subscribe(attributes_callback))
        {
            // Serial.println("Failed to subscribe for shared attribute updates");
            return;
        }

        Serial.println("Subscribe done");

        if (!tb.Shared_Attributes_Request(attribute_shared_request_callback))
        {
            // Serial.println("Failed to request for shared attributes");
            return;
        }
        tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
        
        // Custom Device Attributes
        tb.sendAttributeData("location", "Lab Room");
        tb.sendAttributeData("firmware", "v2.0.0");
        tb.sendAttributeData("model", "DHT22 + AnomalyDetector");
    }
    else if (tb.connected())
    {
        tb.loop();
    }
}

void task_core_iot_run(void *pvParameters)
{
    ProcessedData telemetry_data;
    
    // Khởi tạo Software Timer cho LED 1 (để dùng tính năng chớp nháy)
    xLed1Timer = xTimerCreate("LED 1 Timer", pdMS_TO_TICKS(blinkingInterval), pdTRUE, (void *)0, led1_timer_callback);
    
    while(1)
    {
        // Check if Wi-Fi is connected and we have a server configured
        if (WiFi.status() == WL_CONNECTED && !CORE_IOT_SERVER.isEmpty())
        {
            // Maintain MQTT Connection
            CORE_IOT_reconnect();
            
            // Check for new data in the queue (timeout rất nhỏ để không làm block tb.loop)
            if (tb.connected() && xQueueReceive(xQueueMLToServer, &telemetry_data, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                if (telemetry_data.temperature > -999.0f)
                {
                    tb.sendTelemetryData("temperature", telemetry_data.temperature);
                    tb.sendTelemetryData("humidity", telemetry_data.humidity);
                    tb.sendTelemetryData("anomaly_score", telemetry_data.anomaly_score);
                    
                    String status_str = "NORMAL";
                    if (telemetry_data.status == STATUS_WARNING) status_str = "WARNING";
                    else if (telemetry_data.status == STATUS_DANGER) status_str = "DANGER";
                    
                    tb.sendAttributeData("system_status", status_str.c_str());
                    Serial.println("☁️ Successfully sent Telemetry data to CoreIOT!");
                }
            }
        }
        // Giảm thời gian ngủ của Task để tb.loop() được gọi liên tục (giúp phản hồi RPC ngay lập tức)
        vTaskDelay(pdMS_TO_TICKS(300)); // Yield to other tasks
    }
}