#include "task_wifi.h"
#include "task_check_info.h"

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.printf("[WIFI] AP Started. IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void startSTA()
{
    if (WIFI_SSID.isEmpty()) return;

    WiFi.mode(WIFI_STA);
    if (WIFI_PASS.isEmpty()) WiFi.begin(WIFI_SSID.c_str());
    else WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

    Serial.printf("[WIFI] Connecting to %s", WIFI_SSID.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retry++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
        xSemaphoreGive(xBinarySemaphoreInternet);
    } else {
        Serial.println("[WIFI] Failed to connect");
    }
}

bool Wifi_reconnect()
{
    if (WiFi.status() == WL_CONNECTED) return true;
    Serial.println("[WIFI] Connection lost. Reconnecting...");
    startSTA();
    return false;
}

void task_wifi_manager(void *pvParameters) {
    while(1) {
        if (!WIFI_SSID.isEmpty()) Wifi_reconnect();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
