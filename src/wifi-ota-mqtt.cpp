#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include <PubSubClient.h>
#include "wifi-ota-mqtt.h"
#include "Logger.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMqttConnectionAttempt = 0;
int mqttConnectionAttemptIntervalSeconds = 30;

mqttHandlerType mqttHandler = NULL;

void callback(char* topic, byte* payload, unsigned int length);
void attemptMqtt();

void connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        logger.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    logger.println("Ready");
    char message[100];
    sprintf(message, "IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    logger.println(message);
}

void setupOTA() {
    ArduinoOTA.setHostname("ESP32-ESPNOW-GW-OTA");
    //ArduinoOTA.setPassword("admin");
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        logger.print("Start updating ");
        logger.println(type.c_str());
    });
    ArduinoOTA.onEnd([]() {
        logger.println("End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        logger.print("Ota error: ");
        if (error == OTA_AUTH_ERROR) {
            logger.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            logger.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            logger.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            logger.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            logger.println("End Failed");
        }
    });
    ArduinoOTA.begin();
}

void setupMqtt() {
    mqttClient.setServer(MQTT_SERVER, 1883);
    mqttClient.setCallback(callback);
    attemptMqtt();
}

void attemptMqtt() {
    if (!mqttClient.connected()) {
        logger.println("Attempting connection to MQTT...");
        if (mqttClient.connect("ESP32-ESPNOW-GW-MQTT")) {
            logger.println("connected");
            mqttClient.subscribe(MQTT_SUB_TOPIC);
            mqttClient.publish(MQTT_LOG_TOPIC, "ESP32-ESPNOW-GW-MQTT connected");
            logger.println("MQTT ready");
            // publish IP address once to MQTT_GLOBAL_LOG_TOPIC in meaninful format
            char message[100];
            sprintf(message, "ESP-NOW mqtt gateway IP: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
            mqttClient.publish(MQTT_GLOBAL_LOG_TOPIC, message);
        } else {
            logger.print("failed with state ");
            logger.println(mqttClient.state());
            logger.print("Next attempt in ");
            logger.print(mqttConnectionAttemptIntervalSeconds);
            logger.println(" seconds");
            lastMqttConnectionAttempt = millis();
        }
    }
}

void manageConnections() {
    if (!mqttClient.connected() && millis() - lastMqttConnectionAttempt > mqttConnectionAttemptIntervalSeconds * 1000) {
        attemptMqtt();
    }
    ArduinoOTA.handle();
    mqttClient.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
    logger.print("MQTT message arrived [");
    logger.print(topic);
    logger.print("] ");
    for (int i = 0; i < length; i++) {
        logger.print((char)payload[i]);
    }
    logger.println();
    if (mqttHandler != NULL) {
        mqttHandler(topic, payload, length);
    }
}

void setMqttHandler(mqttHandlerType handler) {
    mqttHandler = handler;
}

void mqttLog(char *message) {
    mqttClient.publish(MQTT_LOG_TOPIC, message);
}

void mqttSend(char *message) {
    mqttClient.publish(MQTT_PUB_TOPIC, message);
}