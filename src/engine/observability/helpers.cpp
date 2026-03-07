#include "helpers.h"
#include "utils/utils.h"

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

std::string koi8r_to_utf8(const std::string& input) {
	if (input.empty()) {
		return input;
	}
	// Fast path: ASCII-only strings need no conversion
	bool has_high = false;
	for (unsigned char c : input) {
		if (c >= 128) {
			has_high = true;
			break;
		}
	}
	if (!has_high) {
		return input;
	}
	// Each KOI8-R byte expands to at most 3 UTF-8 bytes, plus null terminator
	std::string output(input.size() * 3 + 1, '\0');
	koi_to_utf8(const_cast<char*>(input.c_str()), &output[0]);
	output.resize(strlen(output.c_str()));
	return output;
}

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
