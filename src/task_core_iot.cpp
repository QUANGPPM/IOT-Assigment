#include "task_core_iot.h"
#include "app_config.h"
#include "freertos/timers.h"
#include "led_control.h"
#include "global.h"
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char *DEVICE_NAME = "ESP32-DEVICE";
const char *MQTT_PASS = "x";

String tele_topic = String("iot/gateway/") + DEVICE_NAME + "/telemetry";
String attr_topic = String("iot/gateway/") + DEVICE_NAME + "/attributes";
String rpc_req_topic = String("iot/gateway/") + DEVICE_NAME + "/rpc/request/+/+";
String rpc_resp_prefix = String("iot/gateway/") + DEVICE_NAME + "/rpc/response/";

volatile bool ledState = false;
volatile uint16_t blinkingInterval = 1000U;
TimerHandle_t xLed1Timer = NULL;
bool led1_current_state = false;

void led1_timer_callback(TimerHandle_t xTimer) {
  if (ledState) {
    led1_current_state = !led1_current_state;
    led_set_state(1, led1_current_state);
  }
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, message)) return;

  String topicStr = String(topic);
  if (topicStr.indexOf("/rpc/request/") != -1) {
    int lastSlash = topicStr.lastIndexOf('/');
    String rpc_id = topicStr.substring(lastSlash + 1);
    
    String method = doc["method"] | "";
    if (method == "") {
      int methodStart = topicStr.indexOf("/request/") + 9;
      method = topicStr.substring(methodStart, lastSlash);
    }
    
    bool newState = doc.is<bool>() ? doc.as<bool>() : doc["params"];
    Serial.printf("[RPC] Method: %s -> %s\n", method.c_str(), newState ? "ON" : "OFF");

    if (method == "setLED") {
      led_set_state(3, newState);
      mqttClient.publish((rpc_resp_prefix + rpc_id).c_str(), newState ? "1" : "0");
    }
    else if (method == "setLedManual") {
      led_set_state(2, newState);
      mqttClient.publish((rpc_resp_prefix + rpc_id).c_str(), newState ? "1" : "0");
    }
  }
  else if (topicStr.indexOf("/attributes") != -1) {
    if (doc.containsKey("ledState")) {
      ledState = doc["ledState"];
      if (ledState) xTimerStart(xLed1Timer, 0);
      else { xTimerStop(xLed1Timer, 0); led_set_state(1, false); }
      Serial.printf("[IOT] LED1 Blink: %s\n", ledState ? "ON" : "OFF");
    }
    if (doc.containsKey("blinkingInterval")) {
      blinkingInterval = doc["blinkingInterval"];
      xTimerChangePeriod(xLed1Timer, pdMS_TO_TICKS(blinkingInterval), 0);
      Serial.printf("[IOT] Blink Interval: %d ms\n", blinkingInterval);
    }
  }
}

void CORE_IOT_reconnect() {
  if (!mqttClient.connected()) {
    mqttClient.setBufferSize(512);
    mqttClient.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
    mqttClient.setCallback(mqtt_callback);

    if (mqttClient.connect(DEVICE_NAME, CORE_IOT_TOKEN.c_str(), MQTT_PASS)) {
      Serial.println("[IOT] Connected to Gateway");
      mqttClient.subscribe(rpc_req_topic.c_str());
      mqttClient.subscribe((attr_topic + "/update").c_str()); 
      
      StaticJsonDocument<512> attrDoc;
      attrDoc["deviceName"] = DEVICE_NAME;
      attrDoc["led0_desc"] = "Webserver";
      attrDoc["led1_desc"] = "Blink";
      attrDoc["led2_desc"] = "Manual";
      attrDoc["led3_desc"] = "Auto";
      
      String attrMsg;
      serializeJson(attrDoc, attrMsg);
      mqttClient.publish(attr_topic.c_str(), attrMsg.c_str());
    }
  }
}

void task_core_iot_run(void *pvParameters) {
  ProcessedData telemetry_data;
  xLed1Timer = xTimerCreate("LED 1 Timer", pdMS_TO_TICKS(blinkingInterval), pdTRUE, (void *)0, led1_timer_callback);

  while (1) {
    if (WiFi.status() == WL_CONNECTED && !CORE_IOT_SERVER.isEmpty()) {
      CORE_IOT_reconnect();
      mqttClient.loop();

      if (mqttClient.connected() && xQueueReceive(xQueueMLToServer, &telemetry_data, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (telemetry_data.temperature > -999.0f) {
          StaticJsonDocument<256> teleDoc;
          teleDoc["deviceName"] = DEVICE_NAME; 
          teleDoc["temperature"] = telemetry_data.temperature;
          teleDoc["humidity"] = telemetry_data.humidity;
          teleDoc["anomaly_score"] = telemetry_data.anomaly_score;
          
          String teleMsg;
          serializeJson(teleDoc, teleMsg);
          if (mqttClient.publish(tele_topic.c_str(), teleMsg.c_str())) {
            Serial.printf("[IOT] Telemetry Sent (T:%.1f, H:%.1f)\n", telemetry_data.temperature, telemetry_data.humidity);
          }
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}