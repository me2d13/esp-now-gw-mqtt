#include "Logger.h"
#include <Arduino.h> // Needed for millis()
#include <time.h>    // Needed for NTP time

#define USE_NTP true

Logger logger;

// Constructor
Logger::Logger(size_t maxSize)
    : m_maxSize(maxSize) {}

// Add a log entry with a timestamp
std::string Logger::log(const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Create the log entry with a timestamp
    std::string logEntry = getTimestamp() + " - " + message;

    // Add the new log entry
    m_logs.push_back(logEntry);
    // log to serial
    if (!logEntry.empty() && logEntry.back() == '\n') {
        std::string withoutLastChar = logEntry.substr(0, logEntry.length() - 1);
        Serial.println(withoutLastChar.c_str());
    } else {
        Serial.println(logEntry.c_str());
    } 

    // Remove the oldest entry if we exceed the maximum size
    if (m_logs.size() > m_maxSize)
    {
        m_logs.pop_front();
    }
    return logEntry; // Return the log message with timestamp
}

// Retrieve all log entries
std::deque<std::string> Logger::getLogs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs; // Return a copy to avoid external modifications
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

// Helper function to format the timestamp from the RTC (NTP)
std::string Logger::getTimestamp() const
{
    bool useMilis = true;
    struct tm timeinfo;
    if (USE_NTP) {
        if (getLocalTime(&timeinfo)) {
            useMilis = false;
        }
    }
    if (useMilis)
    {
        // return milis formatted to hours:minutes:seconds.miliseconds
        char timestamp[20];
        sprintf(timestamp, "%02d:%02d:%02d.%03d", millis() / 3600000, millis() / 60000 % 60, millis() / 1000 % 60, millis() % 1000);
        return std::string(timestamp);
    }
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(timestamp);
}

void Logger::print(const std::string &message)
{
    // add message to buffer
    for (char c : message)
    {
        m_lineBuffer[m_lineBufferIndex++] = c;
    }
}

void Logger::println()
{
    // add newline to buffer
    m_lineBuffer[m_lineBufferIndex++] = '\n';
    // log buffer
    log(std::string(m_lineBuffer, m_lineBufferIndex));
    // clear buffer
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
