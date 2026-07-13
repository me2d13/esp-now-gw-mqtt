#include "Logger.h"
#include <Arduino.h>
#include <time.h>
#include <memory>
#include "config.h"

#define USE_NTP true

Logger logger;

// Constructor
Logger::Logger(size_t maxSize)
    : m_maxSize(maxSize), m_lineBuffer{0}, m_lineBufferIndex(0), m_logCounter(0), m_lastFlushedCounter(0),
      m_lastFlushTime(0) {
    // Set default appender to SimpleJsonAppender
    m_appender = std::unique_ptr<LogAppender>(new SimpleJsonAppender());
}

// Set the log appender
void Logger::setAppender(std::unique_ptr<LogAppender> appender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appender = std::move(appender);
}

// Add a log entry with a timestamp
LogEntry Logger::log(const std::string &message)
{
    return log(message, "");
}

// Add a log entry with a timestamp and explicit job
LogEntry Logger::log(const std::string &message, const std::string &job) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logCounter++;

    unsigned long currentMillis = millis();

    // Clean control characters from the message
    std::string cleanedMessage;
    cleanedMessage.reserve(message.size());
    for (char c : message) {
        if ((unsigned char)c >= 32 && c != 127) {
            cleanedMessage += c;
        } else if (c == '\t') {
            cleanedMessage += "    "; // Replace tab with spaces
        }
    }

    // Check if NTP time is available
    struct tm timeinfo = {};
    LogEntry entry = (USE_NTP && getLocalTime(&timeinfo))
                         ? LogEntry(m_logCounter, currentMillis, mktime(&timeinfo), cleanedMessage, job) // NTP time available
                         : LogEntry(m_logCounter, currentMillis, cleanedMessage, job); // No NTP time

    m_logs.push_back(entry);
#ifdef LOG_TO_SERIAL
    if (job.empty()) {
        Serial.println((entry.getHumanTimestamp() + " - " + entry.message).c_str());
    } else {
        Serial.println((entry.getHumanTimestamp() + " - [" + job + "] " + entry.message).c_str());
    }
#endif
    if (m_logs.size() > m_maxSize) {
        m_logs.pop_front();
    }
    return entry;
}

// Retrieve all log entries
std::deque<LogEntry> Logger::getLogs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs;
}

// Clear all log entries
void Logger::clearLogs()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}

// Retrieve the size of the log
size_t Logger::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs.size();
}

void Logger::print(const std::string &message)
{
    // Add message to buffer with bounds check
    for (char c : message)
    {
        if (m_lineBufferIndex < (int)sizeof(m_lineBuffer) - 1) {
            m_lineBuffer[m_lineBufferIndex++] = c;
        }
    }
}

void Logger::println()
{
    // Log buffer (without raw \n)
    if (m_lineBufferIndex > 0) {
        log(std::string(m_lineBuffer, m_lineBufferIndex));
    } else {
        log("");
    }
    // Clear buffer
    m_lineBufferIndex = 0;
}

void Logger::println(const std::string &message)
{
    print(message);
    println();
}

void Logger::print(const char *message)
{
    print(std::string(message));
}

void Logger::print(const char message)
{
    print(std::string(1, message));
}

bool Logger::flushLogs() {
    if (!m_appender) return false;

    std::string serializedLogs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        serializedLogs = m_appender->serializeLogs(m_logs, m_lastFlushedCounter);
    }

    bool success = m_appender->flushLogsToEndpoint(serializedLogs);

    if (success) {
        // Update counters only on success
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto &entry: m_logs) {
            if (entry.id > m_lastFlushedCounter) {
                m_lastFlushedCounter = entry.id;
            }
        }
        m_lastFlushTime = millis();
    }

    return success;
}

void Logger::manageLogFlushing() {
    uint32_t unsentCount = 0;
    unsigned long currentTime = millis();
    bool shouldFlush = false;

    // Check conditions under lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Calculate unsent logs count
        unsentCount = m_logCounter - m_lastFlushedCounter;

        // No unsent logs, nothing to do
        if (unsentCount == 0) return;

        // Check if we've reached the count limit
        if (unsentCount >= LOG_FLUSH_COUNT_LIMIT) {
            shouldFlush = true;
        }
        // Check if timeout has expired
        else if (currentTime - m_lastFlushTime >= (LOG_FLUSH_TIMEOUT_S * 1000)) {
            shouldFlush = true;
        }
    }

    // Flush without holding the lock
    if (shouldFlush) {
        flushLogs();
    }
}

// Convenience functions to switch appenders
void Logger::useJsonAppender() {
    auto jsonAppender = std::unique_ptr<LogAppender>(new SimpleJsonAppender());
    setAppender(std::move(jsonAppender));
}

void Logger::useOpenTelemetryAppender(const std::string &serviceName, const std::string &serviceVersion,
                                      const std::string &otlpEndpoint) {
    auto otlpAppender = std::unique_ptr<LogAppender>(
        new OpenTelemetryAppender(serviceName, serviceVersion, otlpEndpoint));
    setAppender(std::move(otlpAppender));
}
