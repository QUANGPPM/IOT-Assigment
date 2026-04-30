#include "task_wifi.h"
#include "task_check_info.h"

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        return;
    }

    WiFi.mode(WIFI_STA);

    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    Serial.print("Connecting to WiFi");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        retry++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
        //Give a semaphore here
        xSemaphoreGive(xBinarySemaphoreInternet);
    }
}

bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        return true;
    }
    Serial.println("WiFi Disconnected. Reconnecting...");
    startSTA();
    return false;
}

void task_wifi_manager(void *pvParameters) {
    while(1) {
        if (!WIFI_SSID.isEmpty()) {
            Wifi_reconnect();
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
