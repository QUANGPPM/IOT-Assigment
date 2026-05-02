#include "task_core_iot.h"
#include "app_config.h"
#include "freertos/timers.h"
#include "led_control.h"
#include <ArduinoJson.h>

// Use PubSubClient directly instead of ThingsBoard library to match Python client protocol
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* DEVICE_NAME = "ESP32-DEVICE";
const char* MQTT_PASS = "x";

// Topics matching Python client
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

// MQTT Callback to handle RPC and Attribute updates (matching Python logic)
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("📥 Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) return;

  String topicStr = String(topic);

  // ── Handle RPC ───────────────────────────────────────────────────
  if (topicStr.indexOf("/rpc/request/") != -1) {
    // Extract RPC ID from topic
    int lastSlash = topicStr.lastIndexOf('/');
    String rpc_id = topicStr.substring(lastSlash + 1);
    String method = doc["method"] | "";
    
    Serial.printf("📡 [RPC] ← id=%s, method=%s\n", rpc_id.c_str(), method.c_str());

    if (method == "setLED") {
      bool newState = doc["params"];
      led_set_state(2, newState);
      led_set_state(3, newState);
      
      // Send response
      String resp_topic = rpc_resp_prefix + rpc_id;
      StaticJsonDocument<128> respDoc;
      respDoc["id"] = rpc_id;
      respDoc["result"] = newState;
      String respMsg;
      serializeJson(respDoc, respMsg);
      mqttClient.publish(resp_topic.c_str(), respMsg.c_str());
    }
  }
  // ── Handle Attributes ─────────────────────────────────────────────
  else if (topicStr.indexOf("/attributes") != -1) {
    if (doc.containsKey("ledState")) {
      ledState = doc["ledState"];
      if (ledState) xTimerStart(xLed1Timer, 0);
      else {
        xTimerStop(xLed1Timer, 0);
        led_set_state(1, false);
      }
    }
    if (doc.containsKey("blinkingInterval")) {
      blinkingInterval = doc["blinkingInterval"];
      xTimerChangePeriod(xLed1Timer, pdMS_TO_TICKS(blinkingInterval), 0);
    }
  }
}

void CORE_IOT_reconnect() {
  if (!mqttClient.connected()) {
    mqttClient.setBufferSize(512);
    mqttClient.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
    mqttClient.setCallback(mqtt_callback);

    Serial.printf("Attempting MQTT connection to Gateway at %s:%d...\n", CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
    // Username is the Token from Web UI, Password is 'x', ClientID is ESP32-DEVICE
    if (mqttClient.connect(DEVICE_NAME, CORE_IOT_TOKEN.c_str(), MQTT_PASS)) {
      Serial.println("✅ Connected!");
      
      // Subscribe to topics matching Python client
      mqttClient.subscribe(rpc_req_topic.c_str());
      String attr_upd_topic = attr_topic + "/update";
      mqttClient.subscribe(attr_upd_topic.c_str()); 
      
      // Send initial attributes
      StaticJsonDocument<256> attrDoc;
      attrDoc["deviceName"] = DEVICE_NAME;
      attrDoc["firmware"] = "v2.0.0";
      attrDoc["model"] = "YoloUno-Physical";
      attrDoc["location"] = "Lab Room";
      String attrMsg;
      serializeJson(attrDoc, attrMsg);
      mqttClient.publish(attr_topic.c_str(), attrMsg.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
}

void task_core_iot_run(void *pvParameters) {
  ProcessedData telemetry_data;
  xLed1Timer = xTimerCreate("LED 1 Timer", pdMS_TO_TICKS(blinkingInterval),
                            pdTRUE, (void *)0, led1_timer_callback);

  while (1) {
    if (WiFi.status() == WL_CONNECTED && !CORE_IOT_SERVER.isEmpty()) {
      CORE_IOT_reconnect();
      mqttClient.loop();

    if (mqttClient.connected() && xQueueReceive(xQueueMLToServer, &telemetry_data, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (telemetry_data.temperature > -999.0f) {
          // Create FLAT payload as requested by Gateway
          StaticJsonDocument<256> teleDoc;
          teleDoc["deviceName"] = DEVICE_NAME; 
          teleDoc["temperature"] = telemetry_data.temperature;
          teleDoc["humidity"] = telemetry_data.humidity;
          teleDoc["anomaly_score"] = telemetry_data.anomaly_score;

          // Send Flat Telemetry (Contains all sensor data)
          String teleMsg;
          serializeJson(teleDoc, teleMsg);
          
          if (mqttClient.publish(tele_topic.c_str(), teleMsg.c_str())) {
            Serial.println("📤 Telemetry & Status sent successfully");
          } else {
            Serial.println("❌ Failed to send data");
          }
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}