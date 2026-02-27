#ifndef BYLINS_OTEL_HELPERS_H
#define BYLINS_OTEL_HELPERS_H

#include "utils/utils_time.h"
#include "utils/timestamp.h"
#include "otel_metrics.h"
#include <string>
#include <map>
#include <memory>

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
 * Convert string from KOI8-R to UTF-8.
 * Safe to call on ASCII strings (pass through unchanged).
 * Used to sanitize all strings before sending to OTEL (protobuf requires UTF-8).
 */
std::string koi8r_to_utf8(const std::string& input);

using utils::NowTs;

} // namespace observability

#endif // BYLINS_OTEL_HELPERS_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
