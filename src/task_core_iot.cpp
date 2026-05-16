#include "task_core_iot.h"
#include "app_config.h"
#include "global.h"
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ─── ThingsBoard Device API Topics ───────────────────────────────────────────
static const char *TOPIC_TELEMETRY      = "v1/devices/me/telemetry";
static const char *TOPIC_ATTR_PUB       = "v1/devices/me/attributes";
static const char *TOPIC_ATTR_SUB       = "v1/devices/me/attributes";
static const char *TOPIC_ATTR_REQ       = "v1/devices/me/attributes/request/1";
static const char *TOPIC_ATTR_RESP      = "v1/devices/me/attributes/response/1";
static const char *TOPIC_RPC_REQUEST    = "v1/devices/me/rpc/request/+";
static const char *TOPIC_RPC_RESP_PFX   = "v1/devices/me/rpc/response/";

// ─── Helpers ─────────────────────────────────────────────────────────────────
static String _extract_rpc_id(const String &topic) {
  int lastSlash = topic.lastIndexOf('/');
  return topic.substring(lastSlash + 1);
}

static void _rpc_respond(const String &request_id, bool success) {
  StaticJsonDocument<64> resp;
  resp["result"] = success ? "ok" : "error";
  String buf;
  serializeJson(resp, buf);
  mqttClient.publish((String(TOPIC_RPC_RESP_PFX) + request_id).c_str(), buf.c_str());
}

/**
 * Hàm xử lý JSON attributes (dùng chung cho cả update realtime và response khi boot)
 */
void _process_shared_attributes(JsonObject root) {
  bool changed = false;

  if (root.containsKey("control_mode")) {
    String mode = root["control_mode"].as<String>();
    g_is_auto_mode = (mode == "AUTO");
    changed = true;
  }

  if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(10)) == pdTRUE) {
    if (root.containsKey("anomaly_threshold_warning")) {
      g_anomaly_warn = root["anomaly_threshold_warning"].as<float>();
      changed = true;
    }
    if (root.containsKey("anomaly_threshold_danger")) {
      g_anomaly_danger = root["anomaly_threshold_danger"].as<float>();
      changed = true;
    }
    xSemaphoreGive(xMutexThresholds);
  }

  if (changed) {
    Serial.printf("[IOT] Shared Attributes updated -> Mode:%s | W:%.1f | D:%.1f\n",
                  g_is_auto_mode ? "AUTO" : "MANUAL", (float)g_anomaly_warn, (float)g_anomaly_danger);
  }
}

// ─── MQTT Callback ────────────────────────────────────────────────────────────
void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, message)) {
    Serial.println("[IOT] JSON parse error");
    return;
  }

  String topicStr = String(topic);
  Serial.printf("[IOT] Recv topic: %s\n", topic);

  // 1. Nhận Shared Attributes cập nhật realtime
  if (topicStr.equals(TOPIC_ATTR_SUB)) {
    _process_shared_attributes(doc.as<JsonObject>());
  }
  
  // 2. Nhận phản hồi Shared Attributes sau khi chủ động xin (Request)
  else if (topicStr.equals(TOPIC_ATTR_RESP)) {
    if (doc.containsKey("shared")) {
      _process_shared_attributes(doc["shared"].as<JsonObject>());
    }
  }

  // 3. Xử lý RPC Request
  else if (topicStr.indexOf("/rpc/request/") != -1) {
    String request_id = _extract_rpc_id(topicStr);
    String method = doc["method"] | "";
    bool params   = doc["params"] | false;

    if (!g_is_auto_mode) {
      if (method == "setFanState" || method == "setMistState") {
        Serial.printf("[RPC] MANUAL cmd -> %s: %s\n", method.c_str(), params ? "ON" : "OFF");
        _rpc_respond(request_id, true);
      } else {
        _rpc_respond(request_id, false);
      }
    } else {
      Serial.println("[RPC] Ignored - system is in AUTO mode");
      _rpc_respond(request_id, false);
    }
  }
}

// ─── Kết nối / Đăng ký MQTT ─────────────────────────────────────────────────
void CORE_IOT_reconnect() {
  if (mqttClient.connected()) return;

  mqttClient.setBufferSize(1024);
  mqttClient.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
  mqttClient.setCallback(mqtt_callback);

  if (mqttClient.connect("ESP32-S3-BaseStation", CORE_IOT_TOKEN.c_str(), "")) {
    Serial.println("[IOT] Connected to CoreIOT");

    // 1. Subscribe các topic cần thiết
    mqttClient.subscribe(TOPIC_ATTR_SUB);   // Nhận update realtime
    mqttClient.subscribe(TOPIC_ATTR_RESP);  // Nhận phản hồi cho request
    mqttClient.subscribe(TOPIC_RPC_REQUEST);

    // 2. Publish Client Attributes (Gửi 1 lần khi boot)
    StaticJsonDocument<256> attrDoc;
    attrDoc["wifi_ssid"]        = WIFI_SSID;
    attrDoc["ip_address"]       = WiFi.localIP().toString();
    attrDoc["firmware_version"] = "TinyML_v1.0";
    attrDoc["model"]            = "DHT20_YoloUno_Node";
    
    String attrMsg;
    serializeJson(attrDoc, attrMsg);
    mqttClient.publish(TOPIC_ATTR_PUB, attrMsg.c_str());

    // 3. Chủ động xin (Request) Shared Attributes hiện tại từ Cloud
    StaticJsonDocument<256> reqDoc;
    reqDoc["sharedKeys"] = "control_mode,anomaly_threshold_warning,anomaly_threshold_danger";
    String reqMsg;
    serializeJson(reqDoc, reqMsg);
    mqttClient.publish(TOPIC_ATTR_REQ, reqMsg.c_str());

    Serial.println("[IOT] Client Attr sent & Shared Attr requested");
  } else {
    Serial.printf("[IOT] Connect failed, rc=%d\n", mqttClient.state());
  }
}

// ─── Task chính ───────────────────────────────────────────────────────────────
void task_core_iot_run(void *pvParameters) {
  ProcessedData telemetry_data;

  while (1) {
    // 1. Chờ dữ liệu mới từ TinyML Task (Data-driven)
    // Task này sẽ treo ở đây cho đến khi có kết quả TinyML mới
    if (xQueueReceive(xQueueMLToServer, &telemetry_data, portMAX_DELAY) == pdTRUE) {
      
      if (WiFi.status() == WL_CONNECTED && !CORE_IOT_SERVER.isEmpty()) {
        CORE_IOT_reconnect();
        mqttClient.loop();

        if (mqttClient.connected() && telemetry_data.temperature > -999.0f) {
          // Logic phân loại environment_status (0:Normal, 1:Warning, 2:Danger)
          int env_status = 0; 
          if (telemetry_data.status == STATUS_DANGER) env_status = 2;
          else if (telemetry_data.status == STATUS_WARNING) env_status = 1;

          StaticJsonDocument<256> teleDoc;
          teleDoc["temperature"]        = telemetry_data.temperature;
          teleDoc["humidity"]           = telemetry_data.humidity;
          teleDoc["anomaly_score"]      = telemetry_data.anomaly_score;
          teleDoc["environment_status"] = env_status;

          String teleMsg;
          serializeJson(teleDoc, teleMsg);
          
          if (mqttClient.publish(TOPIC_TELEMETRY, teleMsg.c_str())) {
            Serial.printf("[IOT] Telemetry sent -> Score:%.2f Status:%d\n", 
                          telemetry_data.anomaly_score, env_status);
          }
        }
      }
    }
    // Không cần vTaskDelay(100) ở đây vì xQueueReceive đã treo task một cách hiệu quả
  }
}