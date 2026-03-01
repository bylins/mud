#include "file_log_sender.h"
#include "utils/logger.h"
#include "engine/core/config.h"
#include "engine/db/global_objects.h"

#include <cstring>

namespace logging {

static void write_log_message(const std::string& message, FILE* file) {
	if (!file) {
		return;
	}
	if (!runtime_config.output_thread() && runtime_config.log_stderr().empty()) {
		fputs(message.c_str(), file);
		fputs("\n", file);
	} else {
		const std::size_t len = message.size();
		std::shared_ptr<char> buffer(new char[len + 1], [](char *p) { delete[] p; });
		memcpy(buffer.get(), message.c_str(), len);
		buffer.get()[len] = '\0';
		GlobalObjects::output_thread().output(OutputThread::message_t{buffer, len, file});
	}
}

FileLogSender::FileLogSender() {
    // Constructor - file handles are managed by runtime_config
}

void FileLogSender::Debug(const std::string& message) {
    Debug(message, {});
}

void FileLogSender::Debug(const std::string& message,
                         const std::map<std::string, std::string>& attributes) {
    write_to_file(message, attributes);
}

void FileLogSender::Info(const std::string& message) {
    Info(message, {});
}

void FileLogSender::Info(const std::string& message,
                        const std::map<std::string, std::string>& attributes) {
    write_to_file(message, attributes);
}

void FileLogSender::Warn(const std::string& message) {
    Warn(message, {});
}

void FileLogSender::Warn(const std::string& message,
                        const std::map<std::string, std::string>& attributes) {
    write_to_file(message, attributes);
}

void FileLogSender::Error(const std::string& message) {
    Error(message, {});
}

void FileLogSender::Error(const std::string& message,
                         const std::map<std::string, std::string>& attributes) {
    write_to_file(message, attributes);
}

void FileLogSender::write_to_file(const std::string& message,
                                  const std::map<std::string, std::string>& attributes) {
    FILE* file = get_log_file(attributes);
    if (!file) {
        return;
    }

    // Message already contains timestamp (formatted by vlog_buffer)
    write_log_message(message, file);
}

FILE* FileLogSender::get_log_file(const std::map<std::string, std::string>& attributes) {
    auto it = attributes.find("log_type");
    const std::string& log_type = (it != attributes.end()) ? it->second : "syslog";

    if (log_type == "syslog")  return runtime_config.logs(SYSLOG).handle();
    if (log_type == "errlog")  return runtime_config.logs(ERRLOG).handle();
    if (log_type == "imlog")   return runtime_config.logs(IMLOG).handle();
    if (log_type == "msdp")    return runtime_config.logs(MSDP_LOG).handle();
    if (log_type == "money")   return runtime_config.logs(MONEY_LOG).handle();

    // Unknown log_type - fall back to syslog
    return runtime_config.logs(SYSLOG).handle();
}

} // namespace logging

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
