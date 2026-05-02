#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Struct to hold sensor data for passing through queues
struct SensorData {
    float temperature;
    float humidity;
};

// Enum for system status
enum SystemStatus {
    STATUS_NORMAL,
    STATUS_WARNING,
    STATUS_DANGER
};

// Struct to hold processed data after ML inference
struct ProcessedData {
    float temperature;
    float humidity;
    float anomaly_score;
    SystemStatus status;
};

#endif // APP_CONFIG_H