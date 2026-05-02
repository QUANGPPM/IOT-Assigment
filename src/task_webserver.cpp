#include "task_webserver.h"
#include "app_config.h"
#include <ArduinoJson.h>

// Initialize AsyncWebServer on port 80
AsyncWebServer server(80);
// Initialize AsyncWebSocket on the "/ws" path
AsyncWebSocket ws("/ws");


bool webserver_isrunning = false;


void Webserver_sendata(String data)
{
    if (ws.count() > 0)
    {
        ws.textAll(data); // Send to all connected clients
        Serial.printf("[WS] 📤 Đã gửi dữ liệu: %s\n", data.c_str());
    }
    else
    {
        Serial.println("[WS] ⚠️ Không có client nào đang kết nối!");
    }
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("[WS] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->opcode == WS_TEXT)
        {
            // Concatenate data chunks to form the complete message
            String message;
            message += String((char *)data).substring(0, len);
            // parseJson(message, true);
            // Pass the message to the handler function
            handleWebSocketMessage(message);
        }
    }
}



void connectWSV()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/styles.css", "text/css"); });
    server.begin();
    ElegantOTA.begin(&server);
    webserver_isrunning = true;
}


void Webserver_stop()
{
    ws.closeAll();
    server.end();
    webserver_isrunning = false;
}


void Webserver_reconnect()
{
    if (!webserver_isrunning)
    {
        connectWSV();
    }
    ElegantOTA.loop();
}


void task_webserver_run(void *pvParameters)
{
    // Start the webserver once. This function will set up all handlers
    // for the web page, WebSocket, and ElegantOTA.
    connectWSV();
    Serial.println("[WEB] 🚀 Web server task started and running.");

    while (1)
    {
        // ElegantOTA needs to be called in a loop to handle OTA updates.
        ElegantOTA.loop();

        // AsyncWebServer handles client connections in the background.
        // This delay yields CPU time to other tasks.
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Task to send sensor data over WebSocket.
 * This is a "Consumer" task that waits for data from the TinyML task.
 */
void task_websocket_sender(void *pvParameters)
{
    ProcessedData received_data;
    while (1)
    {
        // Wait for new processed data from the TinyML task
        if (xQueueReceive(xQueueMLToWeb, &received_data, portMAX_DELAY) == pdTRUE)
        {
            // Don't send invalid data
            if (received_data.temperature > -999.0f)
            {
                StaticJsonDocument<200> doc;
                doc["type"] = "update";
                doc["temp"] = received_data.temperature;
                doc["humi"] = received_data.humidity;
                
                const char *status_str = "NORMAL";
                if (received_data.status == STATUS_WARNING) status_str = "WARNING";
                else if (received_data.status == STATUS_DANGER) status_str = "DANGER";
                
                doc["status"] = status_str;

                String output;
                serializeJson(doc, output);
                Webserver_sendata(output);
            }
        }
    }
}
