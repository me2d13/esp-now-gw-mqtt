#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include <PubSubClient.h>
#include "wifi-ota-mqtt.h"
#include "Logger.h"
#include <time.h>

// NTP server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // GMT offset in seconds (e.g., +1 for UTC+1)
const int daylightOffset_sec = 0; //3600; // Daylight savings offset in seconds (adjust if needed)

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMqttConnectionAttempt = 0;
int mqttConnectionAttemptIntervalSeconds = 30;
unsigned long lastWifiConnectionAttempt = 0;
int wifiReconnectionIntervalSeconds = 30;

mqttHandlerType mqttHandler = NULL;

void callback(char* topic, byte* payload, unsigned int length);
void attemptMqtt();
void attemptWifiReconnection();

void scanAndLogNetworks();

void connectWifi() {
    WiFi.mode(WIFI_STA);

    logger.print("Attempting to connect to WiFi SSID: ");
    logger.println(WIFI_SSID);

    int retryCount = 0;

    while (retryCount < WIFI_MAX_RETRIES) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        logger.print("WiFi connection attempt ");
        logger.print(retryCount + 1);
        logger.print("/");
        logger.println(WIFI_MAX_RETRIES);

        // Wait for connection with timeout
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
            // 30 second timeout
            delay(500);
            logger.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            logger.println();
            logger.println("WiFi connected successfully!");
            char message[100];
            sprintf(message, "IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2],
                    WiFi.localIP()[3]);
            logger.println(message);
            return;
        }

        logger.println();
        logger.print("WiFi connection failed. Status: ");
        logger.println(WiFi.status());

        // Scan and log available networks to help with debugging
        scanAndLogNetworks();

        retryCount++;

        if (retryCount < WIFI_MAX_RETRIES) {
            logger.print("Waiting ");
            logger.print(WIFI_RETRY_DELAY_S);
            logger.println(" seconds before retry...");
            delay(WIFI_RETRY_DELAY_S * 1000);
        }
    }

    // All retry attempts failed, reboot the chip
    logger.print("Failed to connect to WiFi after ");
    logger.print(WIFI_MAX_RETRIES);
    logger.println(" attempts. Rebooting...");
    delay(5000);
    ESP.restart();
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
        if (mqttClient.connect(WHO_AM_I)) {
            char message[100];
            logger.println("connected");
            mqttClient.subscribe(MQTT_SUB_TOPIC);
            sprintf(message, "%s connected on IP: %d.%d.%d.%d", WHO_AM_I, WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
            mqttClient.publish(MQTT_LOG_TOPIC, message);
            logger.println("MQTT ready");
            // publish IP address once to MQTT_GLOBAL_LOG_TOPIC in meaninful format
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

void attemptWifiReconnection() {
    if (WiFi.status() != WL_CONNECTED) {
        logger.println("Attempting WiFi reconnection...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
            delay(500);
            logger.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            logger.println();
            logger.println("WiFi reconnected successfully!");
            char message[100];
            sprintf(message, "IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2],
                    WiFi.localIP()[3]);
            logger.println(message);
        } else {
            logger.println();
            logger.print("WiFi reconnection failed. Status: ");
            logger.println(WiFi.status());
            logger.print("Next attempt in ");
            logger.print(wifiReconnectionIntervalSeconds);
            logger.println(" seconds");
            lastWifiConnectionAttempt = millis();
        }
    }
}

void manageConnections() {
    if (WiFi.status() != WL_CONNECTED && millis() - lastWifiConnectionAttempt > wifiReconnectionIntervalSeconds *
        1000) {
        attemptWifiReconnection();
    }
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && millis() - lastMqttConnectionAttempt >
        mqttConnectionAttemptIntervalSeconds * 1000) {
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

void syncNtp() {
    // Initialize time via NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    logger.log("Time synchronized with NTP server.");
    // Log initial time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        logger.log("Current time: " + std::string(timeStr));
    } else {
        logger.log("Failed to obtain time.");
    }
}

void scanAndLogNetworks() {
    logger.println("Scanning for available WiFi networks...");
    int networkCount = WiFi.scanNetworks();

    if (networkCount == -1) {
        logger.println("WiFi scan failed: Scan in progress (try again later)");
    } else if (networkCount == -2) {
        logger.println("WiFi scan failed: Generic scan failure");
    } else if (networkCount < 0) {
        logger.print("WiFi scan failed with unknown error code: ");
        logger.println(networkCount);
    } else if (networkCount == 0) {
        logger.println("No networks found");
    } else {
        logger.print("Found ");
        logger.print(networkCount);
        logger.println(" networks:");

        for (int i = 0; i < networkCount; ++i) {
            logger.print("  ");
            logger.print(i + 1);
            logger.print(": ");
            logger.print(WiFi.SSID(i).c_str());
            logger.print(" (");
            logger.print(WiFi.RSSI(i));
            logger.print(" dBm) ");
            logger.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
            logger.println();
        }
    }
    // Clean up scan results (only if scan was successful)
    if (networkCount > 0) {
        WiFi.scanDelete();
    }
}
