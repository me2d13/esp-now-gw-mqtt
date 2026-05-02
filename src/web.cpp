#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "web.h"
#include "Logger.h"
#include "apiAbout.h"


AsyncWebServer server(80);


void setupWebServer() {

  // API: Get logs
  server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray logArray = doc.to<JsonArray>();
    
    std::deque<LogEntry> logs = logger.getLogs();
    for (const auto& log : logs) {
        JsonObject obj = logArray.add<JsonObject>();
        obj["id"] = log.id;
        obj["timestamp"] = log.getHumanTimestamp();
        obj["isoTimestamp"] = log.getIsoTimestamp();
        obj["message"] = log.message;
        obj["job"] = log.job;
        obj["isNtp"] = log.systemTimeAvailable;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Get device info
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["name"] = WHO_AM_I;
    doc["version"] = SW_VERSION;
    doc["ip"] = WiFi.localIP().toString();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Send message to Serial
  server.on("/api/send", HTTP_POST, [](AsyncWebServerRequest * request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest * request, uint8_t * data, size_t len, size_t index, size_t total) {
      if (index == 0) {
          // Store message in a temp buffer if needed, but for small messages we can just use data
          char* msg = (char*)malloc(len + 1);
          if (msg) {
              memcpy(msg, data, len);
              msg[len] = '\0';
              sendMessageToSerial(msg, "Web API");
              free(msg);
          }
      }
  });

  // API: About (placeholder)
  server.on("/api/about", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray aboutArray = doc.to<JsonArray>();
    getAboutData(aboutArray);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Fallback for 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });

  server.begin();
}