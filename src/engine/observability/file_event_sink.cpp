#include "event_sink.h"

#include "../../third_party_libs/nlohmann/json.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
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

class FileEventSink : public EventSink {
public:
	explicit FileEventSink(const std::string& path) {
		m_file = std::fopen(path.c_str(), "wb");
		if (!m_file) {
			throw std::runtime_error(
				fmt::format("FileEventSink: cannot open {} for writing", path));
		}
	}

	~FileEventSink() override {
		if (m_file) {
			std::fclose(m_file);
		}
	}

	FileEventSink(const FileEventSink&) = delete;
	FileEventSink& operator=(const FileEventSink&) = delete;

	void Emit(const Event& event) override {
		// Use ordered_json so the key sequence is fully determined by us, not
		// by the JSON library's hash/map ordering. Order: ts, name, then
		// attributes sorted by key alphabetically. Stable output is critical
		// for the reproducibility check (diff between two runs with the same
		// seed).
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

	void Flush() override {
		if (m_file) {
			std::fflush(m_file);
		}
	}

private:
	std::FILE* m_file = nullptr;
};

}  // namespace

std::unique_ptr<EventSink> MakeFileEventSink(const std::string& path) {
	return std::make_unique<FileEventSink>(path);
}

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
