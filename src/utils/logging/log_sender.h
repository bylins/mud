#ifndef BYLINS_LOG_SENDER_H
#define BYLINS_LOG_SENDER_H

#include <string>
#include <map>

namespace logging {

enum class LogLevel {
    kDebug,
    kInfo,
    kWarn,
    kError
};

// Interface for log sending (Null Object Pattern)
class ILogSender {
public:
    virtual ~ILogSender() = default;

    virtual void Debug(const std::string& message) = 0;
    virtual void Debug(const std::string& message,
                      const std::map<std::string, std::string>& attributes) = 0;

    virtual void Info(const std::string& message) = 0;
    virtual void Info(const std::string& message,
                     const std::map<std::string, std::string>& attributes) = 0;

    virtual void Warn(const std::string& message) = 0;
    virtual void Warn(const std::string& message,
                     const std::map<std::string, std::string>& attributes) = 0;

    virtual void Error(const std::string& message) = 0;
    virtual void Error(const std::string& message,
                      const std::map<std::string, std::string>& attributes) = 0;
};

} // namespace logging

#endif // BYLINS_LOG_SENDER_H
