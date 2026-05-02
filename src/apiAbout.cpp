#include "apiAbout.h"
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

void addAboutItem(JsonArray& array, const char* name, const char* description, String value) {
    JsonObject obj = array.add<JsonObject>();
    obj["name"] = name;
    obj["description"] = description;
    obj["value"] = value;
}

void getAboutData(JsonArray& array) {
    // Device info
    addAboutItem(array, "sw_version", "Software Version", SW_VERSION);
    addAboutItem(array, "who_am_i", "Device Name", WHO_AM_I);
    addAboutItem(array, "mac_address", "Mac Address", WiFi.macAddress());
    addAboutItem(array, "ip_address", "IP Address", WiFi.localIP().toString());
    addAboutItem(array, "free_heap", "Free Internal Heap", String(esp_get_free_internal_heap_size()));

    // Chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    addAboutItem(array, "chip_model", "Chip Model", String(chip_info.model));
    addAboutItem(array, "chip_revision", "Chip Revision", String(chip_info.revision));
    addAboutItem(array, "chip_cores", "CPU Cores", String(chip_info.cores));
    
    String features = "";
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) features += "WiFi 802.11bgn ";
    if (chip_info.features & CHIP_FEATURE_BLE) features += "BLE ";
    if (chip_info.features & CHIP_FEATURE_BT) features += "BT ";
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH) features += "Embedded Flash ";
    addAboutItem(array, "chip_features", "Chip Features", features);

    // MQTT info
    addAboutItem(array, "mqtt_server", "MQTT Server", MQTT_SERVER);
    addAboutItem(array, "mqtt_sub", "Control Topic (Subscribed)", MQTT_SUB_TOPIC);
    addAboutItem(array, "mqtt_pub", "State Topic (Published)", MQTT_PUB_TOPIC);
    addAboutItem(array, "mqtt_log", "Log Topic", MQTT_LOG_TOPIC);
    addAboutItem(array, "mqtt_global", "Global Log Topic", MQTT_GLOBAL_LOG_TOPIC);
}
