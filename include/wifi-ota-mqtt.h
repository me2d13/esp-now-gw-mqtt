#pragma once

typedef void (*mqttHandlerType)(char* topic, byte* payload, unsigned int length);

void connectWifi();
void setupOTA();
void setupMqtt();
void manageConnections();
void setMqttHandler(mqttHandlerType handler);
void mqttLog(char *message);
void mqttSend(char *message);
void syncNtp();