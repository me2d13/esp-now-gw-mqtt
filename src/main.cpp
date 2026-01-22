#include <Arduino.h>
#include "wifi-ota-mqtt.h"
#include <TickTwo.h>
#include "Logger.h"
#include "web.h"
#include "config.h"

/*
 * OpenTelemetry Logging Example:
 * 
 * To switch to OpenTelemetry format logging, uncomment and modify the following lines in setup():
 * 
 * // Switch to OpenTelemetry logging
 * logger.useOpenTelemetryAppender(
 *     "esp32-gateway",                                    // Service name
 *     "1.0.0",                                           // Service version
 *     "http://your-otel-collector:4318/v1/logs"          // OTLP endpoint
 * );
 * 
 * Or for more advanced setups:
 * auto otlpAppender = std::make_unique<OpenTelemetryAppender>(
 *     WHO_AM_I,                                          // Use device name as service
 *     SW_VERSION,                                        // Use software version
 *     "http://jaeger:14268/api/traces"                   // Jaeger endpoint
 * );
 * logger.setAppender(std::move(otlpAppender));
 * 
 * To switch back to JSON format:
 * logger.useJsonAppender();
 */

#define __LED_BUILTIN 2

#define PIN_ESPNOW_SERIAL_RX 14
#define PIN_ESPNOW_SERIAL_TX 27

#define SERIAL_BUFFER_SIZE 500
#define SERIAL_BAUDRATE 115200

void blinkOnHandler();
void blinkOffHandler();
void heartbeatHandler();

TickTwo timerBlinkOn(blinkOnHandler, 2000);
TickTwo timerBlinkOff(blinkOffHandler, 100);
TickTwo timerHeartbeat(heartbeatHandler, HEART_BEAT_S * 1000);

HardwareSerial espNowSerial(1);
char espSerialMessageBuffer[SERIAL_BUFFER_SIZE];
int espSerialMessageBufferIndex = 0;


void onMqttMessage(char* topic, byte* payload, unsigned int length);
void setupSerialEspNow();
void handleSerialEspNow();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);
  Serial.begin(115200);
  logger.println("Booting");

#ifdef USE_OPENTELEMETRY_LOGGING
  // Initialize OpenTelemetry logging
  logger.useOpenTelemetryAppender(OTLP_SERVICE_NAME, OTLP_SERVICE_VERSION, OTLP_ENDPOINT);
  logger.println("OpenTelemetry logging initialized");
#endif

  connectWifi();
  syncNtp();
  setupOTA();
  setupMqtt();
  setMqttHandler(onMqttMessage);
  setupWebServer();
  setupSerialEspNow();
  timerBlinkOn.start();
  timerHeartbeat.start();
}

void loop() {
  manageConnections();
  handleSerialEspNow();
  logger.manageLogFlushing();
  timerBlinkOn.update();
  timerBlinkOff.update();
  timerHeartbeat.update();
}

void blinkOnHandler() {
  digitalWrite(LED_BUILTIN, true);
  timerBlinkOff.start();
}

void blinkOffHandler() {
  digitalWrite(LED_BUILTIN, false);
}

void heartbeatHandler() {
  logger.print("Heartbeat from ");
  logger.println(WHO_AM_I);
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  logger.print("MQTT message arrived [");
  logger.print(topic);
  logger.print("]: '");
  char message[length+1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  logger.print(message);
  logger.println("'. Passing to serial to TRANS");
  espNowSerial.println(message);
}

void setupSerialEspNow() {
  espNowSerial.begin(SERIAL_BAUDRATE, SERIAL_8N1, PIN_ESPNOW_SERIAL_RX, PIN_ESPNOW_SERIAL_TX);
  logger.print("Serial1 started, pins RX: ");
  logger.print(PIN_ESPNOW_SERIAL_RX);
  logger.print(", TX: ");
  logger.println(PIN_ESPNOW_SERIAL_TX);
}


void handleSerialEspNow() {
  if (espNowSerial.available()) {
    char incomingByte = espNowSerial.read();
    if (incomingByte == '\n' || espSerialMessageBufferIndex >= SERIAL_BUFFER_SIZE) {
      espSerialMessageBuffer[espSerialMessageBufferIndex] = '\0';
      // if message starts with "DATA:" publish to mqtt
      if (strncmp(espSerialMessageBuffer, "DATA:", 5) == 0) {
        mqttSend(espSerialMessageBuffer+5);
        logger.print("Message from TRANS passed to MQTT: ");
        logger.println(espSerialMessageBuffer);
      } else {
        // anything else is considered as log message from transmitter
        logger.log(espSerialMessageBuffer, "ESP32-ESPNOW-GW-TRANS");
      }
      espSerialMessageBufferIndex = 0;
    } else {
      espSerialMessageBuffer[espSerialMessageBufferIndex] = incomingByte;
      espSerialMessageBufferIndex++;
    }
  }
}
