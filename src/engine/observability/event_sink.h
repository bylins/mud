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

// Global event sink, used by engine code (combat, magic, etc.) that wants to
// emit events without taking a sink through every function signature.
//
// Default is a NoOp sink: production builds without a configured exporter
// see no overhead beyond a virtual call into a no-op. The simulator (and
// later the OTLP exporter) installs a real sink with SetGlobalEventSink().
EventSink& GlobalEventSink();
void SetGlobalEventSink(EventSink* sink);

// Engine-side strings (character names, room descriptions, etc.) live in
// KOI8-R; nlohmann::json validates UTF-8 on serialization and rejects KOI8-R
// bytes. Emitters must convert text fields through this helper before putting
// them into Event.attrs.
std::string EngineStringToUtf8(const std::string& koi8r);

}  // namespace observability

#endif  // BYLINS_EVENT_SINK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
