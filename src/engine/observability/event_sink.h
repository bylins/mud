#ifndef BYLINS_EVENT_SINK_H
#define BYLINS_EVENT_SINK_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>

namespace observability {

using EventAttrValue = std::variant<std::int64_t, double, std::string, bool>;

struct Event {
	std::string name;
	std::int64_t ts_unix_ms = 0;
	std::map<std::string, EventAttrValue> attrs;
};

// Generic raw-event sink. Implementations are hidden behind factories below;
// callers only depend on this interface.
class EventSink {
public:
	virtual ~EventSink() = default;
	virtual void Emit(const Event& event) = 0;
	virtual void Flush() = 0;
};

// Creates a sink that appends one JSON object per line to the given file
// (JSONL). Layout matches the OTLP log record schema. Used by the headless
// balance simulator and as a fallback on non-OTLP servers. Throws on open
// failure.
//
// An OtlpLogsSink factory (forwarding to a collector / Loki) is on the
// backlog and will sit next to this one.
std::unique_ptr<EventSink> MakeFileEventSink(const std::string& path);

}  // namespace observability

#endif  // BYLINS_EVENT_SINK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
