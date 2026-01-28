#ifndef BYLINS_TRACE_SENDER_H
#define BYLINS_TRACE_SENDER_H

#include <string>
#include <memory>

namespace tracing {

// Forward declaration
class ISpan;

// Interface for sending traces
class ITraceSender {
public:
	virtual ~ITraceSender() = default;

	// Create parent span
	virtual std::unique_ptr<ISpan> StartSpan(const std::string& name) = 0;

	// Create child span with parent context
	virtual std::unique_ptr<ISpan> StartChildSpan(
		const std::string& name,
		const ISpan& parent) = 0;
};

// Interface for span (analog of OTEL Span, but through vtable)
class ISpan {
public:
	virtual ~ISpan() = default;

	virtual void End() = 0;
	virtual void AddEvent(const std::string& name) = 0;
	virtual void SetAttribute(const std::string& key, const std::string& value) = 0;
	virtual void SetAttribute(const std::string& key, int64_t value) = 0;
	virtual void SetAttribute(const std::string& key, double value) = 0;

	virtual bool IsValid() const = 0;
};

} // namespace tracing

#endif // BYLINS_TRACE_SENDER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
