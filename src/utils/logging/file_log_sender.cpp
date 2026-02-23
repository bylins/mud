#include "file_log_sender.h"
#include "utils/logger.h"
#include <ctime>

namespace logging {

FileLogSender::FileLogSender() {
    // Constructor - file handles are managed by logger.cpp
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
        return;  // Failed to open file
    }

    // Write timestamp and message
    write_time(file);
    fprintf(file, "%s\n", message.c_str());
    fflush(file);  // Ensure message is written immediately
}

FILE* FileLogSender::get_log_file(const std::map<std::string, std::string>& attributes) {
    // Get log type from attributes
    auto it = attributes.find("log_type");
    if (it == attributes.end()) {
        // No log_type specified - use default syslog
        return logfile;
    }

    const std::string& log_type = it->second;

    // Open corresponding file (lazy open, append mode)
    // Note: These files are never closed during runtime (same as original logger.cpp behavior)
    static FILE* shop_file = nullptr;
    static FILE* olc_file = nullptr;
    static FILE* imm_file = nullptr;
    static FILE* error_file = nullptr;
    static FILE* ip_file = nullptr;

    if (log_type == "shop") {
        if (!shop_file) {
            shop_file = fopen("../log/shop.log", "a");
            if (shop_file) {
                opened_files.push_back(shop_file);
            }
        }
        return shop_file;
    } else if (log_type == "olc") {
        if (!olc_file) {
            olc_file = fopen("../log/olc.log", "a");
            if (olc_file) {
                opened_files.push_back(olc_file);
            }
        }
        return olc_file;
    } else if (log_type == "imm") {
        if (!imm_file) {
            imm_file = fopen("../log/imm.log", "a");
            if (imm_file) {
                opened_files.push_back(imm_file);
            }
        }
        return imm_file;
    } else if (log_type == "error") {
        if (!error_file) {
            error_file = fopen("../log/error.log", "a");
            if (error_file) {
                opened_files.push_back(error_file);
            }
        }
        return error_file;
    } else if (log_type == "ip") {
        if (!ip_file) {
            ip_file = fopen("../log/ip.log", "a");
            if (ip_file) {
                opened_files.push_back(ip_file);
            }
        }
        return ip_file;
    } else if (log_type == "perslog") {
        // Personal logs handled separately in pers_log() - not via this sender
        return nullptr;
    }

    // Unknown log type - use syslog as fallback
    return logfile;
}

} // namespace logging

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
