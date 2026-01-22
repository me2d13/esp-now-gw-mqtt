#include "SimpleJsonAppender.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <algorithm>
#include <sstream>
#include "config.h"

// SimpleJsonAppender::serializeEntry implementation (moved from LogEntry::toJson)
std::string SimpleJsonAppender::serializeEntry(const LogEntry &entry) const {
    // Remove newline characters from message
    std::string cleanedMessage = entry.message;
    cleanedMessage.erase(std::remove(cleanedMessage.begin(), cleanedMessage.end(), '\n'), cleanedMessage.end());
    cleanedMessage.erase(std::remove(cleanedMessage.begin(), cleanedMessage.end(), '\r'), cleanedMessage.end());

    // Escape double quotes
    size_t pos = 0;
    while ((pos = cleanedMessage.find('"', pos)) != std::string::npos) {
        cleanedMessage.replace(pos, 1, "\\\"");
        pos += 2; // Move past the escaped quote
    }

    // Use explicit job if provided, otherwise default to WHO_AM_I
    std::string jobField = entry.job.empty() ? WHO_AM_I : entry.job;

    // Use the human-readable timestamp from LogEntry
    std::string timestamp = entry.getHumanTimestamp();

    std::stringstream json;
    json << "{\"id\":" << entry.id << ",\"timestamp\":\"" << timestamp << "\",\"message\":\"" << cleanedMessage <<
            "\",\"job\":\"" << jobField << "\"}";
    return json.str();
}

// SimpleJsonAppender::serializeLogs implementation (moved from Logger::logsToJson)
std::string SimpleJsonAppender::serializeLogs(const std::deque<LogEntry> &logs, uint32_t fromId) const {
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (const auto &entry: logs) {
        if (entry.id > fromId) {
            if (!first) ss << ",";
            ss << serializeEntry(entry);
            first = false;
        }
    }
    ss << "]";
    return ss.str();
}

// SimpleJsonAppender::flushLogsToEndpoint implementation (moved from Logger::flushLogsToEndpoint)
bool SimpleJsonAppender::flushLogsToEndpoint(const std::string &serializedLogs) {
    if (serializedLogs == "[]") return true; // Nothing to send

    HTTPClient http;
    http.begin(LOG_ENDPOINT_URL);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization(LOG_ENDPOINT_USERNAME, LOG_ENDPOINT_PASSWORD);
    //Serial.println(serializedLogs.c_str());
    int httpCode = http.POST(serializedLogs.c_str());
    bool success = false;

    if (httpCode == 200) {
        success = true;
    } else if (httpCode == 400) {
        // Bad request - don't retry, print body for debugging
        Serial.println("HTTP 400 Bad Request - problematic log data:");
        Serial.println(serializedLogs.c_str());
        success = false; // Return false to indicate the data was bad
    } else {
        http.end();
        return false;
    }

    http.end();
    return success;
}