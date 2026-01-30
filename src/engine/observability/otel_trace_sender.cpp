#include "otel_trace_sender.h"

#ifdef WITH_OTEL
#include "otel_provider.h"
#include "otel_helpers.h"
#include "opentelemetry/trace/provider.h"

namespace tracing {
OtelSpan::OtelSpan(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span)
	: m_span(span)
	, m_scope(span ? opentelemetry::nostd::unique_ptr<opentelemetry::trace::Scope>(
		new opentelemetry::trace::Scope(span)) : nullptr) {}

OtelSpan::OtelSpan(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span, bool create_scope)
	: m_span(span)
	, m_scope(create_scope && span ? opentelemetry::nostd::unique_ptr<opentelemetry::trace::Scope>(
		new opentelemetry::trace::Scope(span)) : nullptr) {}

void OtelSpan::End() {
	if (m_span) {
		m_span->End();
	}
}

void OtelSpan::AddEvent(const std::string& name) {
	if (m_span) {
		std::string utf8_name = observability::Koi8ToUtf8(name.c_str());
		m_span->AddEvent(utf8_name);
	}
}

void OtelSpan::SetAttribute(const std::string& key, const std::string& value) {
	if (m_span) {
		std::string utf8_value = observability::Koi8ToUtf8(value.c_str());
		m_span->SetAttribute(key, utf8_value);
	}
}

void OtelSpan::SetAttribute(const std::string& key, int64_t value) {
	if (m_span) {
		m_span->SetAttribute(key, value);
	}
}

void OtelSpan::SetAttribute(const std::string& key, double value) {
	if (m_span) {
		m_span->SetAttribute(key, value);
	}
}

bool OtelSpan::IsValid() const {
	return m_span != nullptr;
}

opentelemetry::trace::SpanContext OtelSpan::GetContext() const {
	if (m_span) {
		return m_span->GetContext();
	}
	return opentelemetry::trace::SpanContext::GetInvalid();
}

std::unique_ptr<ISpan> OtelTraceSender::StartSpan(const std::string& name) {
	if (observability::OtelProvider::Instance().IsEnabled()) {
		auto tracer = observability::OtelProvider::Instance().GetTracer();
		if (tracer) {
			auto span = tracer->StartSpan(name);
			return std::make_unique<OtelSpan>(span);
		}
	}
	return std::make_unique<NoOpSpan>();
}

std::unique_ptr<ISpan> OtelTraceSender::StartChildSpan(
	const std::string& name,
	const ISpan& parent)
{
	// Downcast to OtelSpan to get context
	const OtelSpan* otel_parent = dynamic_cast<const OtelSpan*>(&parent);
	if (!otel_parent || !otel_parent->IsValid()) {
		return std::make_unique<NoOpSpan>();
	}

	if (observability::OtelProvider::Instance().IsEnabled()) {
		auto tracer = observability::OtelProvider::Instance().GetTracer();
		if (tracer) {
			opentelemetry::trace::StartSpanOptions options;
			options.parent = otel_parent->GetContext();

			auto span = tracer->StartSpan(name, {}, options);
			return std::make_unique<OtelSpan>(span);
		}
	}
	return std::make_unique<NoOpSpan>();
}

} // namespace tracing

#endif // WITH_OTEL

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
