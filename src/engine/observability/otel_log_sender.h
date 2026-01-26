#ifndef BYLINS_OTEL_LOG_SENDER_H
#define BYLINS_OTEL_LOG_SENDER_H

#include "utils/logging/log_sender.h"

#ifdef WITH_OTEL

namespace observability {

// OTEL implementation of log sender
class OtelLogSender : public logging::ILogSender {
public:
    OtelLogSender() = default;
    ~OtelLogSender() override = default;

    void Debug(const std::string& message) override;
    void Debug(const std::string& message,
              const std::map<std::string, std::string>& attributes) override;

    void Info(const std::string& message) override;
    void Info(const std::string& message,
             const std::map<std::string, std::string>& attributes) override;

    void Warn(const std::string& message) override;
    void Warn(const std::string& message,
             const std::map<std::string, std::string>& attributes) override;

    void Error(const std::string& message) override;
    void Error(const std::string& message,
              const std::map<std::string, std::string>& attributes) override;
};

} // namespace observability

#endif // WITH_OTEL

#endif // BYLINS_OTEL_LOG_SENDER_H
