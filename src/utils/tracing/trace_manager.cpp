#include "trace_manager.h"
#include "trace_sender.h"

namespace tracing {

namespace {

class NoOpSpan : public ISpan {
public:
	void End() override {}
	void AddEvent(const std::string&) override {}
	void SetAttribute(const std::string&, const std::string&) override {}
	void SetAttribute(const std::string&, int64_t) override {}
	void SetAttribute(const std::string&, double) override {}
	bool IsValid() const override { return false; }
};

class NoOpTraceSender : public ITraceSender {
public:
	std::unique_ptr<ISpan> StartSpan(const std::string&) override {
		return std::make_unique<NoOpSpan>();
	}

	std::unique_ptr<ISpan> StartChildSpan(const std::string&, const ISpan&) override {
		return std::make_unique<NoOpSpan>();
	}
};

} // namespace

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
