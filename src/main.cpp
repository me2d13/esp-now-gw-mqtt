#include <Arduino.h>
#include "wifi-ota-mqtt.h"
#include <TickTwo.h>
#include "Logger.h"
#include "web.h"

#define __LED_BUILTIN 2

#define PIN_ESPNOW_SERIAL_RX 14
#define PIN_ESPNOW_SERIAL_TX 27

#define SERIAL_BUFFER_SIZE 500
#define SERIAL_BAUDRATE 115200

void blinkOnHandler();
void blinkOffHandler();

TickTwo timerBlinkOn(blinkOnHandler, 2000);
TickTwo timerBlinkOff(blinkOffHandler, 100);

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
  connectWifi();
  syncNtp();
  setupOTA();
  setupMqtt();
  setMqttHandler(onMqttMessage);
  setupWebServer();
  setupSerialEspNow();
  timerBlinkOn.start();
}

void loop() {
  manageConnections();
  handleSerialEspNow();
  timerBlinkOn.update();
  timerBlinkOff.update();
}

void blinkOnHandler() {
  digitalWrite(LED_BUILTIN, true);
  timerBlinkOff.start();
}

void blinkOffHandler() {
  digitalWrite(LED_BUILTIN, false);
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
  logger.println("'. Passing to serial");
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
      logger.print("Message from transmitter: ");
      logger.println(espSerialMessageBuffer);
      espSerialMessageBufferIndex = 0;
      // if message starts with "DATA:" publish to mqtt
      if (strncmp(espSerialMessageBuffer, "DATA:", 5) == 0) {
        mqttSend(espSerialMessageBuffer+5);
      }
    } else {
      espSerialMessageBuffer[espSerialMessageBufferIndex] = incomingByte;
      espSerialMessageBufferIndex++;
    }
  }
}
