#include <task_handler.h>
#include "task_webserver.h"
#include "led_control.h"
#include "neo_blinky.h"
#include "task_check_info.h"

// ─── Helper ──────────────────────────────────────────────────────────────────
static void ws_send_ok(const char *page) {
  String msg = String("{\"status\":\"ok\",\"page\":\"") + page + "\"}";
  ws.textAll(msg);
}

static void ws_send_err(const char *reason) {
  String msg = String("{\"status\":\"error\",\"reason\":\"") + reason + "\"}";
  ws.textAll(msg);
}

// ─── WebSocket Message Dispatcher ────────────────────────────────────────────
void handleWebSocketMessage(String message)
{
    Serial.printf("[WS] Recv: %s\n", message.c_str());
    StaticJsonDocument<512> doc;

    if (deserializeJson(doc, message)) {
        Serial.println("[WS] JSON parse error!");
        ws_send_err("json_parse");
        return;
    }

    String page = doc["page"] | "";

    // ─── 1. Cài đặt WiFi / Token ─────────────────────────────────────────────
    if (page == "setting")
    {
        String ssid   = doc["value"]["ssid"]     | "";
        String pass   = doc["value"]["password"] | "";
        String token  = doc["value"]["token"]    | "";
        String server = doc["value"]["server"]   | "";
        String port   = doc["value"]["port"]     | "";

        if (ssid.isEmpty() || server.isEmpty() || token.isEmpty()) {
            ws_send_err("missing_fields");
            return;
        }
        Serial.printf("[WEB] Saving WiFi: %s | Server: %s\n",
                      ssid.c_str(), server.c_str());
        // Lưu và khởi động lại (hàm này tự restart)
        Save_info_File(ssid, pass, token, server, port);
        return;
    }

    // ─── 2. Cấu hình ngưỡng phần cứng (LED / NeoPixel / Anomaly) ─────────────
    if (page == "thresholds")
    {
        JsonObject v = doc["value"];
        bool updated = false;

        if (xSemaphoreTake(xMutexThresholds, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (v.containsKey("led_warn"))    { g_led_thresh_warn   = v["led_warn"].as<float>();   updated = true; }
            if (v.containsKey("led_danger"))  { g_led_thresh_danger = v["led_danger"].as<float>(); updated = true; }
            if (v.containsKey("neo_dry"))     { g_neo_thresh_dry    = v["neo_dry"].as<float>();    updated = true; }
            if (v.containsKey("neo_wet"))     { g_neo_thresh_wet    = v["neo_wet"].as<float>();    updated = true; }
            if (v.containsKey("ano_warn"))    { g_anomaly_warn      = v["ano_warn"].as<float>();   updated = true; }
            if (v.containsKey("ano_danger"))  { g_anomaly_danger    = v["ano_danger"].as<float>(); updated = true; }
            xSemaphoreGive(xMutexThresholds);
        }

        if (updated) {
            // Lưu vào Flash (không restart)
            Save_thresholds_File();
            Serial.printf("[WEB] Thresholds updated: LED W/D=%.1f/%.1f | NEO D/W=%.1f/%.1f | ANO W/D=%.1f/%.1f\n",
                          (float)g_led_thresh_warn,   (float)g_led_thresh_danger,
                          (float)g_neo_thresh_dry,    (float)g_neo_thresh_wet,
                          (float)g_anomaly_warn,      (float)g_anomaly_danger);
            ws_send_ok("thresholds");
        } else {
            ws_send_err("no_valid_fields");
        }
        return;
    }

    // ─── 3. Kiểm tra phần cứng Diagnostic ────────────────────────────────────
    // Giao diện gửi lệnh test trực tiếp lên LED / NeoPixel để kiểm tra phần cứng
    if (page == "diagnostic")
    {
        String target = doc["value"]["target"] | "";

        if (target == "led") {
            // Test LED: bật LED 0 (WebServer LED) trong 2 giây
            bool state = doc["value"]["state"] | true;
            led_set_state(0, state);
            Serial.printf("[DIAG] LED test → %s\n", state ? "ON" : "OFF");
            ws_send_ok("diagnostic_led");

        } else if (target == "neo") {
            // Test NeoPixel: hiện màu được yêu cầu
            String color = doc["value"]["color"] | "white";
            uint8_t r = 0, g = 0, b = 0;
            if      (color == "red")   { r = 255; }
            else if (color == "green") { g = 255; }
            else if (color == "blue")  { b = 255; }
            else if (color == "white") { r = 255; g = 255; b = 255; }
            else if (color == "off")   { /* all zero */ }
            neo_test_color(r, g, b);
            Serial.printf("[DIAG] NeoPixel test → %s (%d,%d,%d)\n",
                          color.c_str(), r, g, b);
            ws_send_ok("diagnostic_neo");

        } else {
            ws_send_err("unknown_target");
        }
        return;
    }

    // ─── 4. Điều khiển LED trực tiếp (giữ lại tương thích) ──────────────────
    if (page == "device")
    {
        JsonObject value = doc["value"];
        if (!value.containsKey("id")) {
            ws_send_err("missing_id");
            return;
        }
        int id = value["id"];
        if (value.containsKey("status")) {
            bool s = value["status"].as<String>().equalsIgnoreCase("ON");
            led_set_state(id, s);
            Serial.printf("[WEB] LED %d → %s\n", id, s ? "ON" : "OFF");
        } else if (value.containsKey("pwm")) {
            led_set_pwm(id, value["pwm"].as<uint8_t>());
            Serial.printf("[WEB] LED %d PWM → %d\n", id, value["pwm"].as<int>());
        }
        return;
    }

    Serial.printf("[WS] Unknown page: %s\n", page.c_str());
}
