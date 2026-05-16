#include "task_check_info.h"

void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    return;
  }
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
  }
  else
  {
    WIFI_SSID = doc["WIFI_SSID"].as<String>();
    WIFI_PASS = doc["WIFI_PASS"].as<String>();
    CORE_IOT_TOKEN = doc["CORE_IOT_TOKEN"].as<String>();
    CORE_IOT_SERVER = doc["CORE_IOT_SERVER"].as<String>();
    CORE_IOT_PORT = doc["CORE_IOT_PORT"].as<String>();
  }
  file.close();
}

void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT)
{
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = CORE_IOT_PORT;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println("Unable to save the configuration.");
  }
  ESP.restart();
};

bool check_info_File(bool check)
{
  if (!check)
  {
    if (!LittleFS.begin(true))
    {
      Serial.println("❌ Lỗi khởi động LittleFS!");
      return false;
    }
    Load_info_File();
  }
  
  if (WIFI_SSID.isEmpty() && WIFI_PASS.isEmpty())
  {
    if (!check)
    {
      startAP();
    }
    return false;
  }
  return true;
}

// =============================================================================
// Load_thresholds_File — Đọc cấu hình ngưỡng từ Flash
// =============================================================================
void Load_thresholds_File()
{
  File file = LittleFS.open("/thresholds.dat", "r");
  if (!file) {
    Serial.println("[CFG] thresholds.dat not found, using defaults");
    return;
  }
  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, file)) {
    Serial.println("[CFG] thresholds.dat parse error");
    file.close();
    return;
  }
  file.close();

  // Cập nhật biến global dưới bảo vệ mutex
  if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (doc.containsKey("led_warn"))    g_led_thresh_warn   = doc["led_warn"].as<float>();
    if (doc.containsKey("led_danger"))  g_led_thresh_danger = doc["led_danger"].as<float>();
    if (doc.containsKey("neo_dry"))     g_neo_thresh_dry    = doc["neo_dry"].as<float>();
    if (doc.containsKey("neo_wet"))     g_neo_thresh_wet    = doc["neo_wet"].as<float>();
    if (doc.containsKey("ano_warn"))    g_anomaly_warn      = doc["ano_warn"].as<float>();
    if (doc.containsKey("ano_danger"))  g_anomaly_danger    = doc["ano_danger"].as<float>();
    xSemaphoreGive(xMutexThresholds);
  }
  Serial.printf("[CFG] Thresholds loaded: LED W/D=%.1f/%.1f | NEO D/W=%.1f/%.1f | ANO W/D=%.1f/%.1f\n",
                (float)g_led_thresh_warn, (float)g_led_thresh_danger,
                (float)g_neo_thresh_dry,  (float)g_neo_thresh_wet,
                (float)g_anomaly_warn,    (float)g_anomaly_danger);
}

// =============================================================================
// Save_thresholds_File — Ghi ngưỡng hiện tại vào Flash
// =============================================================================
void Save_thresholds_File()
{
  DynamicJsonDocument doc(256);
  // Đọc an toàn qua mutex
  if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(50)) == pdTRUE) {
    doc["led_warn"]   = (float)g_led_thresh_warn;
    doc["led_danger"] = (float)g_led_thresh_danger;
    doc["neo_dry"]    = (float)g_neo_thresh_dry;
    doc["neo_wet"]    = (float)g_neo_thresh_wet;
    doc["ano_warn"]   = (float)g_anomaly_warn;
    doc["ano_danger"] = (float)g_anomaly_danger;
    xSemaphoreGive(xMutexThresholds);
  }
  File file = LittleFS.open("/thresholds.dat", "w");
  if (!file) {
    Serial.println("[CFG] Cannot open thresholds.dat for write");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("[CFG] Thresholds saved to flash");
}