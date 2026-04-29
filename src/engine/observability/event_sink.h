#ifndef BYLINS_EVENT_SINK_H
#define BYLINS_EVENT_SINK_H

#include <cstdint>
#include <map>
#include <string>
#include <variant>

namespace observability {

using EventAttrValue = std::variant<std::int64_t, double, std::string, bool>;

struct Event {
	std::string name;
	std::int64_t ts_unix_ms = 0;
	std::map<std::string, EventAttrValue> attrs;
};

// Generic raw-event sink. Two implementations are planned:
// - FileEventSink: appends one JSON line per event (used by the headless balance simulator
//   and as a fallback on non-OTLP servers).
// - OtlpLogsSink (backlog): forwards events as OTLP log records to a collector / Loki.
class EventSink {
public:
	virtual ~EventSink() = default;
	virtual void Emit(const Event& event) = 0;
	virtual void Flush() = 0;
};

}  // namespace observability

#endif  // BYLINS_EVENT_SINK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
