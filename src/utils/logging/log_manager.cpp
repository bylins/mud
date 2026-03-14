#include "log_manager.h"
#include "file_log_sender.h"
#include "engine/db/global_objects.h"

namespace logging {

LogManager& LogManager::Instance() {
    return GlobalObjects::log_manager();
}

LogManager::LogManager() {
    m_senders.push_back(std::make_unique<FileLogSender>());
}

void LogManager::AddSender(std::unique_ptr<ILogSender> sender) {
    m_senders.push_back(std::move(sender));
}

void LogManager::ClearSenders() {
    m_senders.clear();
}

// Static interface implementations - iterate over all senders
void LogManager::Debug(const std::string& message) {
    for (const auto& sender : Instance().m_senders) {
        sender->Debug(message);
    }
}

void LogManager::Debug(const std::string& message,
                      const std::map<std::string, std::string>& attributes) {
    for (const auto& sender : Instance().m_senders) {
        sender->Debug(message, attributes);
    }
}

void LogManager::Info(const std::string& message) {
    for (const auto& sender : Instance().m_senders) {
        sender->Info(message);
    }
}

void LogManager::Info(const std::string& message,
                     const std::map<std::string, std::string>& attributes) {
    for (const auto& sender : Instance().m_senders) {
        sender->Info(message, attributes);
    }
}

void LogManager::Warn(const std::string& message) {
    for (const auto& sender : Instance().m_senders) {
        sender->Warn(message);
    }
}

void LogManager::Warn(const std::string& message,
                     const std::map<std::string, std::string>& attributes) {
    for (const auto& sender : Instance().m_senders) {
        sender->Warn(message, attributes);
    }
}

void LogManager::Error(const std::string& message) {
    for (const auto& sender : Instance().m_senders) {
        sender->Error(message);
    }
}

void LogManager::Error(const std::string& message,
                      const std::map<std::string, std::string>& attributes) {
    for (const auto& sender : Instance().m_senders) {
        sender->Error(message, attributes);
    }
}

} // namespace logging

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
