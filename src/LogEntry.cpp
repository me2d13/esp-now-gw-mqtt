#include "LogAppender.h"
#include <Arduino.h>
#include <sstream>
#include <iomanip>

// Get timestamp as ISO 8601 string (YYYY-MM-DDTHH:MM:SS.sssZ)
std::string LogEntry::getIsoTimestamp() const {
    if (systemTimeAvailable) {
        // Use system time (NTP synchronized)
        struct tm *timeinfo = gmtime(&systemTime);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);

        // Add milliseconds from the captured millisFromStart value
        // Use the milliseconds component from when the log was created
        int logMillis = (int) (millisFromStart % 1000);

        std::stringstream ss;
        ss << buffer << "." << std::setfill('0') << std::setw(3) << logMillis << "Z";
        return ss.str();
    } else {
        // Use millis from start - create a relative timestamp
        unsigned long totalMillis = millisFromStart;
        unsigned long hours = totalMillis / 3600000;
        unsigned long minutes = (totalMillis % 3600000) / 60000;
        unsigned long seconds = (totalMillis % 60000) / 1000;
        unsigned long millis = totalMillis % 1000;

        std::stringstream ss;
        ss << "1970-01-01T" << std::setfill('0')
                << std::setw(2) << hours << ":"
                << std::setw(2) << minutes << ":"
                << std::setw(2) << seconds << "."
                << std::setw(3) << millis << "Z";
        return ss.str();
    }
}

// Get timestamp as human-readable string (YYYY-MM-DD HH:MM:SS or HH:MM:SS.mmm)
std::string LogEntry::getHumanTimestamp() const {
    if (systemTimeAvailable) {
        // Use system time (NTP synchronized)
        struct tm *timeinfo = localtime(&systemTime);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    } else {
        // Use millis from start
        unsigned long totalMillis = millisFromStart;
        unsigned long hours = totalMillis / 3600000;
        unsigned long minutes = (totalMillis % 3600000) / 60000;
        unsigned long seconds = (totalMillis % 60000) / 1000;
        unsigned long millis = totalMillis % 1000;

        char buffer[32];
        sprintf(buffer, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, millis);
        return std::string(buffer);
    }
}

// Get timestamp as nanoseconds since Unix epoch (for OpenTelemetry)
uint64_t LogEntry::getNanosTimestamp() const {
    if (systemTimeAvailable) {
        // Convert system time to nanoseconds
        uint64_t nanos = (uint64_t) systemTime * 1000000000ULL;

        // Add milliseconds from the captured millisFromStart value as nanoseconds
        // Use the milliseconds component from when the log was created
        nanos += (uint64_t)(millisFromStart % 1000) * 1000000ULL;

        return nanos;
    } else {
        // Use millis from start - approximate as nanoseconds since some epoch
        // Note: This won't be a real Unix timestamp, but will be consistent
        return (uint64_t) millisFromStart * 1000000ULL;
    }
}

// Get timestamp as seconds since Unix epoch (for systems that prefer seconds)
uint64_t LogEntry::getSecondsTimestamp() const {
    if (systemTimeAvailable) {
        return (uint64_t) systemTime;
    } else {
        // Convert millis to approximate seconds
        return (uint64_t)(millisFromStart / 1000);
    }
}