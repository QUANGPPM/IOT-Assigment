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
    appConfig.WIFI_SSID = doc["WIFI_SSID"].as<String>();
    appConfig.WIFI_PASS = doc["WIFI_PASS"].as<String>();
    appConfig.CORE_IOT_TOKEN = doc["CORE_IOT_TOKEN"].as<String>();
    appConfig.CORE_IOT_SERVER = doc["CORE_IOT_SERVER"].as<String>();
    appConfig.CORE_IOT_PORT = doc["CORE_IOT_PORT"].as<String>();
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

  // First, update the global configuration object
  appConfig.WIFI_SSID = wifi_ssid;
  appConfig.WIFI_PASS = wifi_pass;
  appConfig.CORE_IOT_TOKEN = CORE_IOT_TOKEN;
  appConfig.CORE_IOT_SERVER = CORE_IOT_SERVER;
  appConfig.CORE_IOT_PORT = CORE_IOT_PORT;

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = appConfig.WIFI_SSID;
  doc["WIFI_PASS"] = appConfig.WIFI_PASS;
  doc["CORE_IOT_TOKEN"] = appConfig.CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = appConfig.CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = appConfig.CORE_IOT_PORT;

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
  
  if (appConfig.WIFI_SSID.isEmpty() && appConfig.WIFI_PASS.isEmpty())
  {
    if (!check)
    {
      startAP();
    }
    return false;
  }
  return true;
}