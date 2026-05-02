#include <task_handler.h>
#include "led_control.h"

void handleWebSocketMessage(String message)
{
    Serial.printf("[WS] 📩 Nhận dữ liệu: %s\n", message.c_str());
    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("[WS] ❌ Lỗi parse JSON!");
        return;
    }
    JsonObject value = doc["value"];
    if (doc["page"] == "device")
    {
        if (!value.containsKey("id"))
        {
            Serial.println("[WEB] ⚠️ JSON missing LED id");
            return;
        }

        int id = value["id"];
        
        if (value.containsKey("status")) {
            String status = value["status"].as<String>();
            Serial.printf("[WEB] ⚙️ Control LED %d → %s\n", id, status.c_str());
            led_set_state(id, status.equalsIgnoreCase("ON"));
        }
        else if (value.containsKey("pwm")) {
            int pwm = value["pwm"];
            Serial.printf("[WEB] 🔆 Change LED %d brightness → %d\n", id, pwm);
            led_set_pwm(id, pwm);
        }
    }
    else if (doc["page"] == "setting")
    {
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();

        Serial.println("[WEB] 📥 Nhận cấu hình từ WebSocket:");
        Serial.println("SSID: " + WIFI_SSID);
        Serial.println("PASS: " + WIFI_PASS);
        Serial.println("TOKEN: " + CORE_IOT_TOKEN);
        Serial.println("SERVER: " + CORE_IOT_SERVER);
        Serial.println("PORT: " + CORE_IOT_PORT);

        // 👉 Gọi hàm lưu cấu hình
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);

        // Phản hồi lại client (tùy chọn)
        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        ws.textAll(msg);
    }
}
