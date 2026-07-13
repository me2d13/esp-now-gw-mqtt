#include <Arduino.h>
#include "wifi-ota-mqtt.h"
#include <TickTwo.h>
#include "Logger.h"
#include "web.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

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

// Timestamp of the last heartbeat received from the transmitter (0 = never received)
unsigned long lastTransmitterHeartbeatMs = 0;
// JsonDocument allocated in setup() after heap is ready.
// Root cause of earlier crash: default 1.25 MB app partition was too small;
// fixed by switching to min_spiffs.csv (1.875 MB app). 
static JsonDocument* espNowDoc = nullptr;


void onMqttMessage(char* topic, byte* payload, unsigned int length);
void setupSerialEspNow();
void handleSerialEspNow();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);
  Serial.begin(115200);
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS Mount Failed");
  }
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
  espNowDoc = new JsonDocument();
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

void sendMessageToSerial(const char* message, const char* source) {
  // Strip newlines for serial transmission to avoid splitting the message
  String cleanMsg = String(message);
  cleanMsg.replace("\n", "");
  cleanMsg.replace("\r", "");
  
  logger.print("Message arrived via ");
  logger.print(source);
  logger.print(": '");
  logger.print(message);
  logger.println("'. Passing to serial to TRANS");
  
  espNowSerial.println(cleanMsg.c_str());
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  char message[length+1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  
  String source = "MQTT [" + String(topic) + "]";
  sendMessageToSerial(message, source.c_str());
}

void setupSerialEspNow() {
  espNowSerial.begin(SERIAL_BAUDRATE, SERIAL_8N1, PIN_ESPNOW_SERIAL_RX, PIN_ESPNOW_SERIAL_TX);
  logger.print("Serial1 started, pins RX: ");
  logger.print(PIN_ESPNOW_SERIAL_RX);
  logger.print(", TX: ");
  logger.println(PIN_ESPNOW_SERIAL_TX);
}


// Returns the number of milliseconds since the last transmitter heartbeat was received.
// Returns -1 if no heartbeat has been received yet.
long getSecondsSinceLastHeartbeat() {
  if (lastTransmitterHeartbeatMs == 0) return -1;
  return (long)((millis() - lastTransmitterHeartbeatMs) / 1000);
}

void handleSerialEspNow() {
  if (espNowSerial.available()) {
    char incomingByte = espNowSerial.read();
    if (incomingByte == '\n' || espSerialMessageBufferIndex >= SERIAL_BUFFER_SIZE - 1) {
      espSerialMessageBuffer[espSerialMessageBufferIndex] = '\0';

      // Strip trailing \r and \n characters
      int len = espSerialMessageBufferIndex;
      while (len > 0 && (espSerialMessageBuffer[len - 1] == '\r' || espSerialMessageBuffer[len - 1] == '\n')) {
        espSerialMessageBuffer[len - 1] = '\0';
        len--;
      }

      if (len > 0 && espNowDoc != nullptr) {
        espNowDoc->clear();
        DeserializationError err = deserializeJson(*espNowDoc, espSerialMessageBuffer);
        if (err) {
          logger.print("TRANS serial parse error: ");
          logger.println(err.c_str());
          logger.println(espSerialMessageBuffer);
        } else {
          const char* type = (*espNowDoc)["type"] | "";

          if (strcmp(type, "data") == 0) {
            // Forward only the inner message payload to MQTT
            String payload;
            serializeJson((*espNowDoc)["message"], payload);
            mqttSend(const_cast<char*>(payload.c_str()));
            logger.print("TRANS data from ");
            logger.print((*espNowDoc)["mac"] | "?");
            logger.print(" passed to MQTT: ");
            logger.println(payload.c_str());

          } else if (strcmp(type, "log") == 0) {
            const char* from    = (*espNowDoc)["from"]    | "ESP32-ESPNOW-GW-TRANS";
            const char* message = (*espNowDoc)["message"] | "";
            logger.log(message, from);

          } else if (strcmp(type, "heartbeat") == 0) {
            lastTransmitterHeartbeatMs = millis();

          } else if (strcmp(type, "response") == 0) {
            const char* command = (*espNowDoc)["command"] | "?";
            const char* status  = (*espNowDoc)["status"]  | "?";
            const char* message = (*espNowDoc)["message"] | "";
            char buf[120];
            snprintf(buf, sizeof(buf), "TRANS response: cmd=%s status=%s %s", command, status, message);
            logger.log(buf, "ESP32-ESPNOW-GW-TRANS");

          } else {
            logger.print("TRANS unknown type: ");
            logger.println(type);
          }
        }
      }

      espSerialMessageBufferIndex = 0;
    } else {
      espSerialMessageBuffer[espSerialMessageBufferIndex] = incomingByte;
      espSerialMessageBufferIndex++;
    }
  }
}
