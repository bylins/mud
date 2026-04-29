#include "file_event_sink.h"

#include <fmt/format.h>

#include <stdexcept>
#include <string>

namespace observability {

namespace {

void AppendJsonString(std::string& out, const std::string& s) {
	out.push_back('"');
	for (char c : s) {
		switch (c) {
			case '"':  out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) < 0x20) {
					out += fmt::format("\\u{:04x}", static_cast<unsigned char>(c));
				} else {
					out.push_back(c);
				}
		}
	}
	out.push_back('"');
}

void AppendAttrValue(std::string& out, const EventAttrValue& v) {
	std::visit([&out](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, std::int64_t>) {
			out += fmt::format("{}", arg);
		} else if constexpr (std::is_same_v<T, double>) {
			out += fmt::format("{}", arg);
		} else if constexpr (std::is_same_v<T, bool>) {
			out += arg ? "true" : "false";
		} else if constexpr (std::is_same_v<T, std::string>) {
			AppendJsonString(out, arg);
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
	std::string line;
	line.reserve(128 + event.attrs.size() * 32);
	line.push_back('{');
	line += "\"ts\":";
	line += fmt::format("{}", event.ts_unix_ms);
	line += ",\"name\":";
	AppendJsonString(line, event.name);
	for (const auto& [k, v] : event.attrs) {
		line.push_back(',');
		AppendJsonString(line, k);
		line.push_back(':');
		AppendAttrValue(line, v);
	}
	line += "}\n";
	std::fwrite(line.data(), 1, line.size(), m_file);
}

void FileEventSink::Flush() {
	if (m_file) {
		std::fflush(m_file);
	}
}

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
