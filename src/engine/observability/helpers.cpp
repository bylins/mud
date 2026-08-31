#include "helpers.h"

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

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
