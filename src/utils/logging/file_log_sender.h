#ifndef BYLINS_FILE_LOG_SENDER_H
#define BYLINS_FILE_LOG_SENDER_H

#include "log_sender.h"
#include <cstdio>

namespace logging {

// File-based log sender (writes to actual log files on disk)
class FileLogSender : public ILogSender {
public:
    FileLogSender();
    ~FileLogSender() override = default;

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

private:
    void write_to_file(const std::string& message,
                      const std::map<std::string, std::string>& attributes);

    FILE* get_log_file(const std::map<std::string, std::string>& attributes);
};

} // namespace logging

#endif // BYLINS_FILE_LOG_SENDER_H
