#ifndef BYLINS_OTEL_HELPERS_H
#define BYLINS_OTEL_HELPERS_H

#include "utils/utils_time.h"
#include "utils/tracing/trace_sender.h"
#include "otel_metrics.h"
#include <string>
#include <map>
#include <memory>

#ifdef WITH_OTEL
#include "opentelemetry/trace/span_context.h"
#endif

namespace observability {

/**
 * RAII wrapper for automatic metric timing.
 * Records histogram metric with duration on destruction.
 *
 * Example:
 *   {
 *       ScopedMetric metric("operation.duration", {{"type", "combat"}});
 *       // ... operation ...
 *   } // automatically records metric
 */
class ScopedMetric {
public:
	ScopedMetric(const std::string& name, const std::map<std::string, std::string>& attrs = {});
	~ScopedMetric();

	// Get elapsed time so far (without ending the metric)
	double elapsed_seconds() const;

private:
	std::string m_name;
	std::map<std::string, std::string> m_attrs;
	utils::CExecutionTimer m_timer;
};

/**
 * RAII wrapper for creating spans in two traces simultaneously.
 * Used for overlapping trace scenarios (e.g., heartbeat + combat).
 *
 * Creates two spans:
 * - Heartbeat span: child of current runtime context (automatically via Scope)
 * - Secondary span: child of provided parent context (NO Scope - doesn't affect runtime context)
 *
 * Example:
 *   DualSpan round_span(
 *       "Combat round",                    // name in heartbeat trace
 *       "Round #5",                        // name in secondary trace
 *       combat_root_span->GetContext()     // parent for secondary span
 *   );
 *   round_span.SetAttribute("round_number", 5);
 *   // ... processing ...
 *   // both spans automatically closed on destruction
 */
class DualSpan {
public:
	/**
	 * @param heartbeat_name Name for span in heartbeat trace
	 * @param secondary_name Name for span in secondary trace
	 * @param secondary_parent Parent context for secondary span
	 */
	DualSpan(const std::string& heartbeat_name,
	         const std::string& secondary_name,
	         const void* secondary_parent);  // opaque pointer to avoid OTEL dependency in header

	~DualSpan();

	// Set attributes on BOTH spans
	void SetAttribute(const std::string& key, const std::string& value);
	void SetAttribute(const std::string& key, int64_t value);
	void SetAttribute(const std::string& key, double value);

	// Add events to BOTH spans
	void AddEvent(const std::string& name);

	// End both spans explicitly (called automatically by destructor)
	void End();

	// Access individual spans (for advanced use)
	tracing::ISpan* heartbeat();
	tracing::ISpan* secondary();

private:
	std::unique_ptr<tracing::ISpan> m_heartbeat_span;
	std::unique_ptr<tracing::ISpan> m_secondary_span;
	bool m_ended;
};

#ifdef WITH_OTEL

/**
 * Get trace ID as hex string from span.
 * Returns empty string if span is invalid or not OtelSpan.
 */
std::string GetTraceId(const tracing::ISpan* span);

/**
 * Set baggage value in current context.
 * Baggage propagates through context and appears in logs/spans.
 *
 * Example:
 *   auto scope = SetBaggage("combat_id", "1738034567_alice_vs_bob");
 *   log("Attack started"); // will have combat_id in attributes
 */
class BaggageScope {
public:
	explicit BaggageScope(const std::string& key, const std::string& value);
	~BaggageScope();

	BaggageScope(const BaggageScope&) = delete;
	BaggageScope& operator=(const BaggageScope&) = delete;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

/**
 * Get baggage value from current context.
 * Returns empty string if key not found.
 */
std::string GetBaggage(const std::string& key);

#else // !WITH_OTEL

// Stub for when OTel is disabled - allows CharData to compile with unique_ptr<BaggageScope>
class BaggageScope {
public:
	BaggageScope(const std::string&, const std::string&) {}
	~BaggageScope() = default;

	BaggageScope(const BaggageScope&) = delete;
	BaggageScope& operator=(const BaggageScope&) = delete;
};

#endif // WITH_OTEL

} // namespace observability

#endif // BYLINS_OTEL_HELPERS_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
