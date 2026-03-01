#include "otel_log_sender.h"

#ifdef WITH_OTEL

#include <string>

#include "otel_helpers.h"
#include "otel_provider.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/logs/logger.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/span_context.h"
#include "opentelemetry/trace/trace_id.h"
#include "opentelemetry/trace/span_id.h"
#include "opentelemetry/nostd/span.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/baggage/baggage.h"
#include "opentelemetry/baggage/baggage_context.h"

namespace logs_api = opentelemetry::logs;

namespace observability {

// Helper: get SpanContext from current active span
static opentelemetry::trace::SpanContext GetCurrentSpanContext() {
	auto context_value = opentelemetry::context::RuntimeContext::GetValue(opentelemetry::trace::kSpanKey);
	auto span_ptr = opentelemetry::nostd::get_if<opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>>(&context_value);
	if (!span_ptr || !(*span_ptr)) {
		return opentelemetry::trace::SpanContext::GetInvalid();
	}
	return (*span_ptr)->GetContext();
}

// Helper: add trace context and user attributes to log record
static void AddAttributesToLogRecord(
	opentelemetry::nostd::unique_ptr<opentelemetry::logs::LogRecord>& log_record,
	const std::map<std::string, std::string>& user_attributes)
{
	if (!log_record) {
		return;
	}

	// Set native OTEL trace context on the log record.
	// This populates the standard OTLP LogRecord fields (trace_id, span_id, flags)
	// which Loki stores as structured metadata and Grafana can filter by.
	auto span_context = GetCurrentSpanContext();
	if (span_context.IsValid()) {
		log_record->SetTraceId(span_context.trace_id());
		log_record->SetSpanId(span_context.span_id());
		log_record->SetTraceFlags(span_context.trace_flags());

		// Also store as string attribute for Loki derivedFields (Loki->Tempo link)
		char trace_id_hex[33] = {};
		span_context.trace_id().ToLowerBase16(opentelemetry::nostd::span<char, 32>(trace_id_hex, 32));
		log_record->SetAttribute("trace_id", std::string(trace_id_hex, 32));
	}

	// Add baggage values (combat_trace_id, quest_trace_id, etc.)
	auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
	auto baggage = opentelemetry::baggage::GetBaggage(current_ctx);
	if (baggage) {
		baggage->GetAllEntries([&log_record](opentelemetry::nostd::string_view key,
		                                      opentelemetry::nostd::string_view value) {
			std::string key_str(key.data(), key.size());
			std::string value_str(koi8r_to_utf8(std::string(value.data(), value.size())));
			log_record->SetAttribute(key_str, value_str);
			return true; // continue iteration
		});
	}

	// Add user attributes
	for (const auto& [key, value] : user_attributes) {
		log_record->SetAttribute(key, koi8r_to_utf8(value));
	}
}

static opentelemetry::logs::Severity to_otel_level(logging::LogLevel level) {
	switch (level) {
		case logging::LogLevel::kDebug: return opentelemetry::logs::Severity::kDebug;
		case logging::LogLevel::kInfo: return opentelemetry::logs::Severity::kInfo;
		case logging::LogLevel::kWarn: return opentelemetry::logs::Severity::kWarn;
		case logging::LogLevel::kError: return opentelemetry::logs::Severity::kError;
		default: return opentelemetry::logs::Severity::kInfo;
	}
}

// Helper: log with any level
static void LogWithLevel(logging::LogLevel level,
						const std::string& message,
						const std::map<std::string, std::string>& attributes) {
	if (OtelProvider::Instance().IsEnabled()) {
		auto logger = logs_api::Provider::GetLoggerProvider()->GetLogger("bylins-logger", "", "", "");
		if (logger) {
			auto log_record = logger->CreateLogRecord();
			if (log_record) {
				log_record->SetSeverity(to_otel_level(level));

				// Strip timestamp prefix "YYYY-MM-DD HH:MM:SS.mmm :: " added by format_log_message.
				// OTEL log records carry their own timestamp metadata.
				const auto sep = message.find(" :: ");
				const std::string body = (sep != std::string::npos) ? message.substr(sep + 4) : message;
				log_record->SetBody(koi8r_to_utf8(body));

				// Automatically add trace context + user attributes
				AddAttributesToLogRecord(log_record, attributes);

				logger->EmitLogRecord(std::move(log_record));
			}
		}
	}
}

// All methods now delegate to LogWithLevel
void OtelLogSender::Debug(const std::string& message) {
	LogWithLevel(logging::LogLevel::kDebug, message, {});
}

void OtelLogSender::Debug(const std::string& message,
						 const std::map<std::string, std::string>& attributes) {
	LogWithLevel(logging::LogLevel::kDebug, message, attributes);
}

void OtelLogSender::Info(const std::string& message) {
	LogWithLevel(logging::LogLevel::kInfo, message, {});
}

void OtelLogSender::Info(const std::string& message,
						const std::map<std::string, std::string>& attributes) {
	LogWithLevel(logging::LogLevel::kInfo, message, attributes);
}

void OtelLogSender::Warn(const std::string& message) {
	LogWithLevel(logging::LogLevel::kWarn, message, {});
}

void OtelLogSender::Warn(const std::string& message,
						const std::map<std::string, std::string>& attributes) {
	LogWithLevel(logging::LogLevel::kWarn, message, attributes);
}

void OtelLogSender::Error(const std::string& message) {
	LogWithLevel(logging::LogLevel::kError, message, {});
}

void OtelLogSender::Error(const std::string& message,
						 const std::map<std::string, std::string>& attributes) {
	LogWithLevel(logging::LogLevel::kError, message, attributes);
}

} // namespace observability

#endif // WITH_OTEL

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
