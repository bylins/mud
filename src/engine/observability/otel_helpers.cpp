#include "otel_helpers.h"
#include <iconv.h>

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
	// Cache iconv descriptor globally - game loop is single-threaded
	struct IconvHandle {
		iconv_t cd;
		IconvHandle() : cd(iconv_open("UTF-8", "KOI8-R")) {}
		~IconvHandle() { if (cd != (iconv_t)-1) { iconv_close(cd); } }
	};
	static IconvHandle handle;

	if (handle.cd == (iconv_t)-1) {
		// iconv unavailable: replace non-ASCII bytes with '?'
		std::string safe;
		safe.reserve(input.size());
		for (unsigned char c : input) {
			safe += (c < 128) ? static_cast<char>(c) : '?';
		}
		return safe;
	}
	// Reset shift state from any previous call
	iconv(handle.cd, nullptr, nullptr, nullptr, nullptr);

	const size_t out_size = input.size() * 4;
	std::string output(out_size, '\0');
	char* in_ptr = const_cast<char*>(input.data());
	char* out_ptr = &output[0];
	size_t in_left = input.size();
	size_t out_left = out_size;
	if (iconv(handle.cd, &in_ptr, &in_left, &out_ptr, &out_left) == (size_t)-1) {
		// Replace non-ASCII bytes with '?' to guarantee valid UTF-8 output
		std::string safe;
		safe.reserve(input.size());
		for (unsigned char c : input) {
			safe += (c < 128) ? static_cast<char>(c) : '?';
		}
		return safe;
	}
	output.resize(out_size - out_left);
	return output;
}

} // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
