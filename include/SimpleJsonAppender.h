#ifndef SIMPLE_JSON_APPENDER_H
#define SIMPLE_JSON_APPENDER_H

#include "LogAppender.h"

// Simple JSON appender implementation (current behavior)
class SimpleJsonAppender : public LogAppender {
public:
    SimpleJsonAppender() = default;

    std::string serializeEntry(const LogEntry &entry) const override;

    std::string serializeLogs(const std::deque<LogEntry> &logs, uint32_t fromId) const override;

    bool flushLogsToEndpoint(const std::string &serializedLogs) override;
};

#endif // SIMPLE_JSON_APPENDER_H