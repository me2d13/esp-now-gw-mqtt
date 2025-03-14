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
#define WIFI_SSID "the_sid"
#define WIFI_PASSWORD "the_password"
#define MQTT_SERVER "192.168.1.1"
// interval in seconds to send heartbeat message
#define HEART_BEAT_S 60*60
// interval in seconds to blink built-in LED
#define BLICK_S 30

// version which appears on the web page
#define SW_VERSION "0.2" 

/* CHANGELOG 

0.1 - initial version
0.2 - added NTP support & improved logging

*/

#endif