#include "otel_log_sender.h"

#ifdef WITH_OTEL

#include "otel_provider.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/logs/logger.h"

namespace observability {

static opentelemetry::logs::Severity to_otel_level(logging::LogLevel level) {
    switch (level) {
        case logging::LogLevel::DEBUG: return opentelemetry::logs::Severity::kDebug;
        case logging::LogLevel::INFO: return opentelemetry::logs::Severity::kInfo;
        case logging::LogLevel::WARN: return opentelemetry::logs::Severity::kWarn;
        case logging::LogLevel::ERROR: return opentelemetry::logs::Severity::kError;
        default: return opentelemetry::logs::Severity::kInfo;
    }
}

void OtelLogSender::Debug(const std::string& message) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            logger->EmitLogRecord(to_otel_level(logging::LogLevel::DEBUG), message);
        }
    }
}

void OtelLogSender::Debug(const std::string& message,
                         const std::map<std::string, std::string>& attributes) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            auto log_record = logger->CreateLogRecord();
            if (log_record) {
                log_record->SetSeverity(to_otel_level(logging::LogLevel::DEBUG));
                log_record->SetBody(message);

                for (const auto& [key, value] : attributes) {
                    log_record->SetAttribute(key, value);
                }

                logger->EmitLogRecord(std::move(log_record));
            }
        }
    }
}

void OtelLogSender::Info(const std::string& message) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            logger->EmitLogRecord(to_otel_level(logging::LogLevel::INFO), message);
        }
    }
}

void OtelLogSender::Info(const std::string& message,
                        const std::map<std::string, std::string>& attributes) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            auto log_record = logger->CreateLogRecord();
            if (log_record) {
                log_record->SetSeverity(to_otel_level(logging::LogLevel::INFO));
                log_record->SetBody(message);

                for (const auto& [key, value] : attributes) {
                    log_record->SetAttribute(key, value);
                }

                logger->EmitLogRecord(std::move(log_record));
            }
        }
    }
}

void OtelLogSender::Warn(const std::string& message) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            logger->EmitLogRecord(to_otel_level(logging::LogLevel::WARN), message);
        }
    }
}

void OtelLogSender::Warn(const std::string& message,
                        const std::map<std::string, std::string>& attributes) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            auto log_record = logger->CreateLogRecord();
            if (log_record) {
                log_record->SetSeverity(to_otel_level(logging::LogLevel::WARN));
                log_record->SetBody(message);

                for (const auto& [key, value] : attributes) {
                    log_record->SetAttribute(key, value);
                }

                logger->EmitLogRecord(std::move(log_record));
            }
        }
    }
}

void OtelLogSender::Error(const std::string& message) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            logger->EmitLogRecord(to_otel_level(logging::LogLevel::ERROR), message);
        }
    }
}

void OtelLogSender::Error(const std::string& message,
                         const std::map<std::string, std::string>& attributes) {
    if (OtelProvider::Instance().IsEnabled()) {
        auto logger = OtelProvider::Instance().GetLogger();
        if (logger) {
            auto log_record = logger->CreateLogRecord();
            if (log_record) {
                log_record->SetSeverity(to_otel_level(logging::LogLevel::ERROR));
                log_record->SetBody(message);

                for (const auto& [key, value] : attributes) {
                    log_record->SetAttribute(key, value);
                }

                logger->EmitLogRecord(std::move(log_record));
            }
        }
    }
}

} // namespace observability

#endif // WITH_OTEL

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
