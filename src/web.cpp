#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "web.h"
#include "Logger.h"


AsyncWebServer server(80);


void setupWebServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    // get free mem
    uint32_t freeMem = esp_get_free_internal_heap_size();
    // get chip info using esp_chip_info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    // create response
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title>", request->url().c_str());
    response->print("</head><body><h2>Hello ");
    response->print(request->client()->remoteIP());
    response->print("</h2>");
    response->print("<h3>General</h3>");
    response->print("<ul>");
    response->printf("<li>Version: HTTP/1.%u</li>", request->version());
    response->printf("<li>Method: %s</li>", request->methodToString());
    response->printf("<li>URL: %s</li>", request->url().c_str());
    response->printf("<li>Host: %s</li>", request->host().c_str());
    response->printf("<li>ContentType: %s</li>", request->contentType().c_str());
    response->printf("<li>ContentLength: %u</li>", request->contentLength());
    response->printf("<li>Multipart: %s</li>", request->multipart()?"true":"false");
    response->print("</ul>");
    response->print("<h3>Device</h3>");
    response->print("<ul>");
    response->print("<li>Version: " SW_VERSION "</li>");
    response->printf("<li>Mac address: %s</li>", WiFi.macAddress().c_str());
    response->printf("<li>IP address: %s</li>", WiFi.localIP().toString().c_str());
    response->printf("<li>Free heap: %u</li>", freeMem);
    response->printf("<li>Chip <a href=\"https://github.com/espressif/esp-idf/blob/v5.4/components/esp_hw_support/include/esp_chip_info.h\">model</a>: %d</li>", chip_info.model);
    response->printf("<li>Chip revision: %u</li>", chip_info.revision);
    response->printf("<li>Chip cores: %u</li>", chip_info.cores);
    response->printf("<li>Chip features: %s</li>", (chip_info.features & CHIP_FEATURE_WIFI_BGN ? "802.11bgn" : "none"));
    response->printf("<li>Chip features: %s</li>", (chip_info.features & CHIP_FEATURE_BLE ? "BLE" : "none"));
    response->printf("<li>Chip features: %s</li>", (chip_info.features & CHIP_FEATURE_BT ? "BT" : "none"));
    response->printf("<li>Chip features: %s</li>", (chip_info.features & CHIP_FEATURE_EMB_FLASH ? "Embedded Flash" : "No Embedded Flash"));
    response->printf("<li>Chip features: %s</li>", (chip_info.features & CHIP_FEATURE_EMB_PSRAM ? "Embedded PSRAM" : "No Embedded PSRAM"));
    //
    
    response->print("</ul>");
    response->print("<h3>MQTT</h3>");
    response->print("<ul>");
    response->printf("<li>Control topic (this device subsribed to): %s</li>", MQTT_SUB_TOPIC);
    response->printf("<li>State topic (this device send messages to): %s</li>", MQTT_PUB_TOPIC);
    response->printf("<li>Log topic (this device sends log messages to): %s</li>", MQTT_LOG_TOPIC);
    response->printf("<li>Global log topic (this device sends IP address on boot): %s</li>", MQTT_GLOBAL_LOG_TOPIC);
    response->print("</ul>");
    response->print("<h3>Logs</h3>");
    response->print("<ul>");
    // iterate over logs
    std::deque<std::string> logs = logger.getLogs();
    for (const auto& log : logs) {
        response->print("<li>");
        response->print(log.c_str());
        response->print("</li>\n");
    }
    response->print("</ul>");
    response->print("</body></html>");
    //send the response last
    request->send(response);
  });

  server.begin();
}