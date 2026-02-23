#ifndef BYLINS_NOOP_TRACE_SENDER_H
#define BYLINS_NOOP_TRACE_SENDER_H

#include "trace_sender.h"

namespace tracing {

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

} // namespace tracing

#endif // BYLINS_NOOP_TRACE_SENDER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
