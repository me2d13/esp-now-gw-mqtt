#pragma once

void setupWebServer();
void sendMessageToSerial(const char* message, const char* source);

// Defined in main.cpp — seconds since last transmitter heartbeat, -1 if never received
long getSecondsSinceLastHeartbeat();
