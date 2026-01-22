#ifndef LOG_APPENDER_H
#define LOG_APPENDER_H

#include <deque>
#include <string>
#include <cstdint>
#include <ctime>

// Represents a single log entry
class LogEntry {
public:
    LogEntry(uint32_t id, unsigned long millisFromStart, const std::string &message)
        : id(id), millisFromStart(millisFromStart), message(message), job(""), systemTimeAvailable(false) {
        systemTime = 0;
    }

    LogEntry(uint32_t id, unsigned long millisFromStart, const std::string &message, const std::string &job)
        : id(id), millisFromStart(millisFromStart), message(message), job(job), systemTimeAvailable(false) {
        systemTime = 0;
    }

    LogEntry(uint32_t id, unsigned long millisFromStart, time_t systemTime, const std::string &message)
        : id(id), millisFromStart(millisFromStart), message(message), job(""), systemTime(systemTime),
          systemTimeAvailable(true) {
    }

    LogEntry(uint32_t id, unsigned long millisFromStart, time_t systemTime, const std::string &message,
             const std::string &job)
        : id(id), millisFromStart(millisFromStart), message(message), job(job), systemTime(systemTime),
          systemTimeAvailable(true) {
    }

    uint32_t id; // log entry sequence number
    unsigned long millisFromStart; // millis() at time of log creation
    time_t systemTime; // system time (from NTP) if available
    bool systemTimeAvailable; // true if systemTime is valid
    std::string message;
    std::string job; // job field, empty string means use default (WHO_AM_I)

    // Helper methods for appenders
    // Get timestamp as ISO 8601 string (YYYY-MM-DDTHH:MM:SS.sssZ)
    std::string getIsoTimestamp() const;

    // Get timestamp as human-readable string (YYYY-MM-DD HH:MM:SS or HH:MM:SS.mmm)
    std::string getHumanTimestamp() const;

    // Get timestamp as nanoseconds since Unix epoch (for OpenTelemetry)
    uint64_t getNanosTimestamp() const;

    // Get timestamp as seconds since Unix epoch (for systems that prefer seconds)
    uint64_t getSecondsTimestamp() const;

    // future extension: std::string level, std::string loggerName
};

// Interface for log appenders that handle endpoint-specific formatting and sending
class LogAppender {
public:
    virtual ~LogAppender() = default;

    // Serialize a single log entry to the appropriate format
    virtual std::string serializeEntry(const LogEntry &entry) const = 0;

    // Serialize a collection of log entries to the appropriate format
    virtual std::string serializeLogs(const std::deque<LogEntry> &logs, uint32_t fromId) const = 0;

    // Send logs to the endpoint
    virtual bool flushLogsToEndpoint(const std::string &serializedLogs) = 0;
};

#endif // LOG_APPENDER_H