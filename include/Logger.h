#ifndef LOGGER_H
#define LOGGER_H

#include <deque>
#include <string>
#include <mutex>
#include <sstream>
#include <Arduino.h>
#include <cstdint>
#include <memory>
#include "LogAppender.h"
#include "SimpleJsonAppender.h"
#include "OpenTelemetryAppender.h"

class Logger {
public:
    explicit Logger(size_t maxSize = 100); // Constructor with a maximum size

    // Set the log appender (takes ownership)
    void setAppender(std::unique_ptr<LogAppender> appender);

    // Add a log entry with a timestamp and return the log entry
    LogEntry log(const std::string &message);

    // Add a log entry with a timestamp and explicit job field
    LogEntry log(const std::string &message, const std::string &job);

    void print(const std::string &message);

    void println(const std::string &message);

    void print(const char *message);

    void print(const char message);

    void print(int value) { print(std::to_string(value)); }
    void println(int value) { println(std::to_string(value)); }

    void println();

    // Retrieve all log entries
    std::deque<LogEntry> getLogs() const;

    // Clear all log entries
    void clearLogs();

    // Retrieve the size of the log
    size_t size() const;

    // Flush unsent logs using the configured appender
    bool flushLogs();

    // Manage log flushing based on time and count thresholds
    void manageLogFlushing();

    // Convenience functions to switch appenders
    void useJsonAppender();

    void useOpenTelemetryAppender(const std::string &serviceName, const std::string &serviceVersion,
                                  const std::string &otlpEndpoint);

private:
    size_t m_maxSize; // Maximum number of log entries
    mutable std::mutex m_mutex; // Protect shared data
    std::deque<LogEntry> m_logs; // Store log entries
    char m_lineBuffer[300]; // Buffer for formatting log entries
    int m_lineBufferIndex = 0; // Index in the buffer
    uint32_t m_logCounter = 0; // log entry sequence number
    uint32_t m_lastFlushedCounter = 0; // last flushed log entry sequence number
    unsigned long m_lastFlushTime = 0; // last time logs were flushed (millis)
    std::unique_ptr<LogAppender> m_appender; // Log appender for endpoint-specific handling
};

extern Logger logger;

#endif // LOGGER_H