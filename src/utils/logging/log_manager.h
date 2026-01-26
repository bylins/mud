#ifndef BYLINS_LOG_MANAGER_H
#define BYLINS_LOG_MANAGER_H

#include "log_sender.h"
#include <vector>
#include <memory>

namespace logging {

// Central logging manager - coordinates all log senders
class LogManager {
public:
    static LogManager& Instance();

    // Add a log sender to the list
    void AddSender(std::unique_ptr<ILogSender> sender);

    // Clear all senders
    void ClearSenders();

    // Get current senders (for inspection)
    const std::vector<std::unique_ptr<ILogSender>>& GetSenders() const { return m_senders; }

    // Static logging interface (delegates to all registered senders)
    static void Debug(const std::string& message);
    static void Debug(const std::string& message,
                     const std::map<std::string, std::string>& attributes);

    static void Info(const std::string& message);
    static void Info(const std::string& message,
                    const std::map<std::string, std::string>& attributes);

    static void Warn(const std::string& message);
    static void Warn(const std::string& message,
                    const std::map<std::string, std::string>& attributes);

    static void Error(const std::string& message);
    static void Error(const std::string& message,
                     const std::map<std::string, std::string>& attributes);

private:
    LogManager();
    ~LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::vector<std::unique_ptr<ILogSender>> m_senders;
};

} // namespace logging

#endif // BYLINS_LOG_MANAGER_H
