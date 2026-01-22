// rename this file to config.h and update values below

#ifndef CONFIG_H
#define CONFIG_H

#define WHO_AM_I "ESP32-ESPNOW-GW-MQTT"

// this device listens at topic below
#define MQTT_SUB_TOPIC "/esp-now/gw/send"
// this device sends messages to topic below
#define MQTT_PUB_TOPIC "/esp-now/gw/receive"
// logging topic
#define MQTT_LOG_TOPIC "/esp-now/gw/log"
// logging topic to send IP once
#define MQTT_GLOBAL_LOG_TOPIC "log"
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define MQTT_SERVER "192.168.1.1"
// interval in seconds to send heartbeat message
#define HEART_BEAT_S (60*60)
// interval in seconds to blink built-in LED
#define BLICK_S 30

// Log flushing configuration
#define LOG_FLUSH_TIMEOUT_S 30      // Flush after this many seconds if there are unsent logs
#define LOG_FLUSH_COUNT_LIMIT 10    // Flush immediately when unsent logs reach this count

// version which appears on the web page
#define SW_VERSION "0.2" 

// Logging endpoint configuration (for SimpleJsonAppender)
#define LOG_ENDPOINT_URL "http://your-server:5080/api/default/espnow/_json"
#define LOG_ENDPOINT_USERNAME "your-username@example.com"
#define LOG_ENDPOINT_PASSWORD "your-log-endpoint-password"
#define LOG_TO_SERIAL  // Comment out to disable serial logging

// OpenTelemetry configuration (uncomment to use OpenTelemetryAppender instead of JSON)
// #define USE_OPENTELEMETRY_LOGGING
// #define OTLP_ENDPOINT "http://your-otel-collector:4318/v1/logs"
// #define OTLP_SERVICE_NAME WHO_AM_I
// #define OTLP_SERVICE_VERSION SW_VERSION

// WiFi connection retry configuration
#define WIFI_RETRY_DELAY_S 10       // Wait time between WiFi connection attempts in seconds
#define WIFI_MAX_RETRIES 5          // Maximum number of connection attempts before reboot

// Example MQTT test commands:
// mosquitto_pub -h YOUR_MQTT_SERVER -t "/esp-now/gw/send" -m "{\"to\":\"YOUR_DEVICE_MAC\",\"message\":{\"channel\":1, \"push\":500}}"

/* CHANGELOG 

0.1 - initial version
0.2 - added NTP support & improved logging, OpenTelemetry support, WiFi retry logic

*/

#endif