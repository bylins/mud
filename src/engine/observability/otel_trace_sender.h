#ifndef BYLINS_OTEL_TRACE_SENDER_H
#define BYLINS_OTEL_TRACE_SENDER_H

#include "utils/tracing/trace_sender.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/nostd/unique_ptr.h"

namespace tracing {

class OtelSpan : public ISpan {
public:
	explicit OtelSpan(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span);
	OtelSpan(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span, bool create_scope);

	void End() override;
	void AddEvent(const std::string& name) override;
	void SetAttribute(const std::string& key, const std::string& value) override;
	void SetAttribute(const std::string& key, int64_t value) override;
	void SetAttribute(const std::string& key, double value) override;
	bool IsValid() const override;

	opentelemetry::trace::SpanContext GetContext() const;

private:
	opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
	opentelemetry::nostd::unique_ptr<opentelemetry::trace::Scope> m_scope;
};

class OtelTraceSender : public ITraceSender {
public:
	std::unique_ptr<ISpan> StartSpan(const std::string& name) override;
	std::unique_ptr<ISpan> StartChildSpan(const std::string& name, const ISpan& parent) override;
};

} // namespace tracing
#endif // BYLINS_OTEL_TRACE_SENDER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
