#include "file_event_sink.h"

#include "../../third_party_libs/nlohmann/json.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace observability {

namespace {

nlohmann::ordered_json AttrValueToJson(const EventAttrValue& v) {
	return std::visit([](auto&& arg) -> nlohmann::ordered_json {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, std::int64_t>) {
			return arg;
		} else if constexpr (std::is_same_v<T, double>) {
			return arg;
		} else if constexpr (std::is_same_v<T, bool>) {
			return arg;
		} else if constexpr (std::is_same_v<T, std::string>) {
			return arg;
		}
	}, v);
}

}  // namespace

FileEventSink::FileEventSink(const std::string& path)
	: m_path(path) {
	m_file = std::fopen(path.c_str(), "wb");
	if (!m_file) {
		throw std::runtime_error(fmt::format("FileEventSink: cannot open {} for writing", path));
	}
}

FileEventSink::~FileEventSink() {
	if (m_file) {
		std::fclose(m_file);
		m_file = nullptr;
	}
}

void FileEventSink::Emit(const Event& event) {
	// Use ordered_json so the key sequence is fully determined by us, not by
	// the JSON library's hash/map ordering. Order: ts, name, then attributes
	// sorted by key alphabetically. Stable output is critical for the
	// reproducibility check (diff between two runs with the same seed).
	nlohmann::ordered_json j;
	j["ts"] = event.ts_unix_ms;
	j["name"] = event.name;
	std::vector<std::string> keys;
	keys.reserve(event.attrs.size());
	for (const auto& kv : event.attrs) {
		keys.push_back(kv.first);
	}
	std::sort(keys.begin(), keys.end());
	for (const auto& k : keys) {
		j[k] = AttrValueToJson(event.attrs.at(k));
	}
	const std::string line = j.dump() + "\n";
	std::fwrite(line.data(), 1, line.size(), m_file);
}

void FileEventSink::Flush() {
	if (m_file) {
		std::fflush(m_file);
	}
}

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
