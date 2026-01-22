#ifndef OPENTELEMETRY_APPENDER_H
#define OPENTELEMETRY_APPENDER_H

#include "LogAppender.h"
#include <cstdint>

// OpenTelemetry appender implementation
class OpenTelemetryAppender : public LogAppender {
private:
    std::string m_serviceName;
    std::string m_serviceVersion;
    std::string m_otlpEndpoint;

public:
    OpenTelemetryAppender(const std::string &serviceName, const std::string &serviceVersion,
                          const std::string &otlpEndpoint);

    std::string serializeEntry(const LogEntry &entry) const override;

    std::string serializeLogs(const std::deque<LogEntry> &logs, uint32_t fromId) const override;

    bool flushLogsToEndpoint(const std::string &serializedLogs) override;

private:
    // Escape JSON string
    std::string escapeJsonString(const std::string &str) const;
};

#endif // OPENTELEMETRY_APPENDER_H