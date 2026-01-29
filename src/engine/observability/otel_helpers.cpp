#include "otel_helpers.h"
#include "utils/tracing/trace_manager.h"
#include "utils/tracing/noop_trace_sender.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"

#ifdef WITH_OTEL
#include "otel_trace_sender.h"
#include "otel_provider.h"
#include "opentelemetry/baggage/baggage_context.h"
#include "opentelemetry/context/context.h"
#include "opentelemetry/trace/span_context.h"
#include "opentelemetry/trace/trace_id.h"
#include <sstream>
#include <iomanip>
#endif

namespace observability {

//
// ScopedMetric
//

ScopedMetric::ScopedMetric(const std::string& name, const std::map<std::string, std::string>& attrs)
	: m_name(name)
	, m_attrs(attrs)
	, m_timer() {}

ScopedMetric::~ScopedMetric() {
	auto duration = m_timer.delta().count();
	OtelMetrics::RecordHistogram(m_name, duration, m_attrs);
}

double ScopedMetric::elapsed_seconds() const {
	return m_timer.delta().count();
}

//
// DualSpan
//

DualSpan::DualSpan(const std::string& heartbeat_name,
                   const std::string& secondary_name,
                   const void* secondary_parent)
	: m_ended(false)
{
	// Create heartbeat span (child of current runtime context via Scope)
	m_heartbeat_span = tracing::TraceManager::Instance().StartSpan(heartbeat_name);

#ifdef WITH_OTEL
	// Create secondary span (NO Scope - doesn't affect runtime context)
	if (secondary_parent) {
		const auto* parent_ctx = static_cast<const opentelemetry::trace::SpanContext*>(secondary_parent);

		auto* sender = dynamic_cast<tracing::OtelTraceSender*>(&tracing::TraceManager::Instance().GetSender());
		if (sender && parent_ctx->IsValid()) {
			// We need to create a child span without Scope
			// Use the low-level OTEL API directly
			auto tracer = observability::OtelProvider::Instance().GetTracer();
			if (tracer) {
				opentelemetry::trace::StartSpanOptions options;
				options.parent = *parent_ctx;

				auto otel_span = tracer->StartSpan(secondary_name, {}, options);
				// Create OtelSpan WITHOUT Scope (don't call ITraceSender::StartSpan which creates Scope)
				m_secondary_span = std::make_unique<tracing::OtelSpan>(otel_span, false); // false = no Scope
			}
		}
	}
#endif

	// If secondary span creation failed, create a no-op span
	if (!m_secondary_span || !m_secondary_span->IsValid()) {
		m_secondary_span = std::make_unique<tracing::NoOpSpan>();
	}
}

DualSpan::~DualSpan() {
	if (!m_ended) {
		End();
	}
}

void DualSpan::SetAttribute(const std::string& key, const std::string& value) {
	if (m_heartbeat_span) {
		m_heartbeat_span->SetAttribute(key, value);
	}
	if (m_secondary_span) {
		m_secondary_span->SetAttribute(key, value);
	}
}

void DualSpan::SetAttribute(const std::string& key, int64_t value) {
	if (m_heartbeat_span) {
		m_heartbeat_span->SetAttribute(key, value);
	}
	if (m_secondary_span) {
		m_secondary_span->SetAttribute(key, value);
	}
}

void DualSpan::SetAttribute(const std::string& key, double value) {
	if (m_heartbeat_span) {
		m_heartbeat_span->SetAttribute(key, value);
	}
	if (m_secondary_span) {
		m_secondary_span->SetAttribute(key, value);
	}
}

void DualSpan::AddEvent(const std::string& name) {
	if (m_heartbeat_span) {
		m_heartbeat_span->AddEvent(name);
	}
	if (m_secondary_span) {
		m_secondary_span->AddEvent(name);
	}
}

void DualSpan::End() {
	if (!m_ended) {
		if (m_heartbeat_span) {
			m_heartbeat_span->End();
		}
		if (m_secondary_span) {
			m_secondary_span->End();
		}
		m_ended = true;
	}
}

tracing::ISpan* DualSpan::heartbeat() {
	return m_heartbeat_span.get();
}

tracing::ISpan* DualSpan::secondary() {
	return m_secondary_span.get();
}

#ifdef WITH_OTEL

//
// Helper functions
//

std::string GetTraceId(const tracing::ISpan* span) {
	if (!span || !span->IsValid()) {
		return "";
	}

	const auto* otel_span = dynamic_cast<const tracing::OtelSpan*>(span);
	if (!otel_span) {
		return "";
	}

	auto ctx = otel_span->GetContext();
	if (!ctx.IsValid()) {
		return "";
	}

	auto trace_id = ctx.trace_id();
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (auto byte : trace_id.Id()) {
		ss << std::setw(2) << static_cast<int>(byte);
	}
	return ss.str();
}

//
// BaggageScope
//

struct BaggageScope::Impl {
	Impl(opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> t) : token(std::move(t)) {}
	opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> token;
};

BaggageScope::BaggageScope(const std::string& key, const std::string& value)
{
	auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
	auto baggage = opentelemetry::baggage::GetBaggage(current_ctx);

	// Create new baggage with added key-value
	auto new_baggage = baggage->Set(key, value);

	// Set new context with updated baggage
	auto mutable_ctx = current_ctx;
	auto new_ctx = opentelemetry::baggage::SetBaggage(mutable_ctx, new_baggage);
	auto token = opentelemetry::context::RuntimeContext::Attach(new_ctx);
	
	m_impl = std::make_unique<Impl>(std::move(token));
}

BaggageScope::~BaggageScope() {
	if (m_impl) {
		opentelemetry::context::RuntimeContext::Detach(*m_impl->token);
	}
}

std::string GetBaggage(const std::string& key) {
	auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
	auto baggage = opentelemetry::baggage::GetBaggage(current_ctx);
	std::string value;
	if (baggage->GetValue(key, value)) {
		return value;
	}
	return "";
}

std::string Koi8ToUtf8(const char* koi8r_str) {
	if (!koi8r_str) {
		return "";
	}

	// Buffer for UTF-8 output (KOI8-R char can expand to 2 bytes in UTF-8)
	char utf8_buffer[kMaxSockBuf * 2];
	char input_buffer[kMaxSockBuf * 2];

	// Copy input to modifiable buffer (koi_to_utf8 modifies input pointer)
	strncpy(input_buffer, koi8r_str, sizeof(input_buffer) - 1);
	input_buffer[sizeof(input_buffer) - 1] = '\0';

	// Convert using existing utility function
	koi_to_utf8(input_buffer, utf8_buffer);

	return std::string(utf8_buffer);
}

#endif // WITH_OTEL

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
