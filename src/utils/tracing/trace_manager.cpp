#include "trace_manager.h"
#include "noop_trace_sender.h"

namespace tracing {

TraceManager& TraceManager::Instance() {
	static TraceManager instance;
	return instance;
}

TraceManager::TraceManager() {
	// By default - NoOp sender
	m_sender = std::make_unique<NoOpTraceSender>();
}

void TraceManager::SetSender(std::unique_ptr<ITraceSender> sender) {
	if (sender) {
		m_sender = std::move(sender);
	}
}

ITraceSender& TraceManager::GetSender() {
	return *m_sender;
}

std::unique_ptr<ISpan> TraceManager::StartSpan(const std::string& name) {
	return m_sender->StartSpan(name);
}

std::unique_ptr<ISpan> TraceManager::StartChildSpan(const std::string& name, const ISpan& parent) {
	return m_sender->StartChildSpan(name, parent);
}

} // namespace tracing

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
