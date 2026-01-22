#include "OpenTelemetryAppender.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <algorithm>
#include <sstream>
#include <time.h>

// OpenTelemetryAppender implementation
OpenTelemetryAppender::OpenTelemetryAppender(const std::string &serviceName, const std::string &serviceVersion,
                                             const std::string &otlpEndpoint)
    : m_serviceName(serviceName), m_serviceVersion(serviceVersion), m_otlpEndpoint(otlpEndpoint) {
}

std::string OpenTelemetryAppender::serializeEntry(const LogEntry &entry) const {
    std::stringstream otlp;

    // Get timestamp as nanoseconds - no parsing needed!
    uint64_t timeNanos = entry.getNanosTimestamp();

    // Escape the message
    std::string escapedMessage = escapeJsonString(entry.message);

    // Use job field or service name
    std::string serviceName = entry.job.empty() ? m_serviceName : entry.job;

    // Create OTLP log record
    otlp << "{"
            << "\"timeUnixNano\":\"" << timeNanos << "\","
            << "\"severityNumber\":9," // INFO level
            << "\"severityText\":\"INFO\","
            << "\"body\":{"
            << "\"stringValue\":\"" << escapedMessage << "\""
            << "},"
            << "\"attributes\":["
            << "{"
            << "\"key\":\"service.name\","
            << "\"value\":{\"stringValue\":\"" << serviceName << "\"}"
            << "},"
            << "{"
            << "\"key\":\"service.version\","
            << "\"value\":{\"stringValue\":\"" << m_serviceVersion << "\"}"
            << "},"
            << "{"
            << "\"key\":\"log.id\","
            << "\"value\":{\"intValue\":\"" << entry.id << "\"}"
            << "},"
            << "{"
            << "\"key\":\"source.timestamp\","
            << "\"value\":{\"stringValue\":\"" << entry.getHumanTimestamp() << "\"}"
            << "},"
            << "{"
            << "\"key\":\"millis.from.start\","
            << "\"value\":{\"intValue\":\"" << entry.millisFromStart << "\"}"
            << "},"
            << "{"
            << "\"key\":\"ntp.available\","
            << "\"value\":{\"boolValue\":" << (entry.systemTimeAvailable ? "true" : "false") << "}"
            << "}"
            << "]"
            << "}";

    return otlp.str();
}

std::string OpenTelemetryAppender::serializeLogs(const std::deque<LogEntry> &logs, uint32_t fromId) const {
    std::stringstream otlp;

    // Get current IP address and MAC address (convert Arduino String to std::string)
    std::string ipAddress = WiFi.localIP().toString().c_str();
    std::string macAddress = WiFi.macAddress().c_str();

    // Create OTLP export request structure
    otlp << "{"
            << "\"resourceLogs\":["
            << "{"
            << "\"resource\":{"
            << "\"attributes\":["
            << "{"
            << "\"key\":\"service.name\","
            << "\"value\":{\"stringValue\":\"" << m_serviceName << "\"}"
            << "},"
            << "{"
            << "\"key\":\"service.version\","
            << "\"value\":{\"stringValue\":\"" << m_serviceVersion << "\"}"
            << "},"
            << "{"
            << "\"key\":\"telemetry.sdk.name\","
            << "\"value\":{\"stringValue\":\"esp32-logger\"}"
            << "},"
            << "{"
            << "\"key\":\"telemetry.sdk.version\","
            << "\"value\":{\"stringValue\":\"1.0.0\"}"
            << "},"
            << "{"
            << "\"key\":\"host.name\","
            << "\"value\":{\"stringValue\":\"" << ipAddress << "\"}"
            << "},"
            << "{"
            << "\"key\":\"host.ip\","
            << "\"value\":{\"stringValue\":\"" << ipAddress << "\"}"
            << "},"
            << "{"
            << "\"key\":\"host.mac\","
            << "\"value\":{\"stringValue\":\"" << macAddress << "\"}"
            << "},"
            << "{"
            << "\"key\":\"device.type\","
            << "\"value\":{\"stringValue\":\"esp32\"}"
            << "},"
            << "{"
            << "\"key\":\"device.model.name\","
            << "\"value\":{\"stringValue\":\"ESP32\"}"
            << "}"
            << "]"
            << "},"
            << "\"scopeLogs\":["
            << "{"
            << "\"scope\":{"
            << "\"name\":\"esp32-logger\","
            << "\"version\":\"1.0.0\""
            << "},"
            << "\"logRecords\":[";

    bool first = true;
    for (const auto &entry: logs) {
        if (entry.id > fromId) {
            if (!first) otlp << ",";
            otlp << serializeEntry(entry);
            first = false;
        }
    }

    otlp << "]"
            << "}"
            << "]"
            << "}"
            << "]"
            << "}";

    return otlp.str();
}

bool OpenTelemetryAppender::flushLogsToEndpoint(const std::string &serializedLogs) {
    // Check if there are no logs to send
    if (serializedLogs.find("\"logRecords\":[]") != std::string::npos) {
        return true; // No logs to send
    }

    HTTPClient http;
    http.begin(m_otlpEndpoint.c_str());
    http.addHeader("Content-Type", "application/json");

    // Add any authentication headers if needed
    // http.addHeader("Authorization", "Bearer your-token");

    int httpCode = http.POST(serializedLogs.c_str());
    bool success = false;

    if (httpCode >= 200 && httpCode < 300) {
        success = true;
    } else if (httpCode == 400) {
        // Bad request - print for debugging
        Serial.println("OTLP HTTP 400 Bad Request - problematic log data:");
        Serial.println(serializedLogs.c_str());
        success = false;
    } else {
        Serial.print("OTLP export failed with HTTP code: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }

    http.end();
    return success;
}

std::string OpenTelemetryAppender::escapeJsonString(const std::string &str) const {
    std::string escaped = str;

    // Remove newline characters
    escaped.erase(std::remove(escaped.begin(), escaped.end(), '\n'), escaped.end());
    escaped.erase(std::remove(escaped.begin(), escaped.end(), '\r'), escaped.end());

    // Escape double quotes
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2; // Move past the escaped quote
    }

    // Escape backslashes
    pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        // Skip already escaped quotes
        if (pos + 1 < escaped.length() && escaped[pos + 1] == '"') {
            pos += 2;
            continue;
        }
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }

    return escaped;
}