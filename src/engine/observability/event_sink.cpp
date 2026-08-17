#include "event_sink.h"
#include "utils/utils_encoding.h"

#include "utils/utils.h"

#include <algorithm>
#include <array>
#include <vector>

namespace observability {

namespace {

std::vector<EventSink*>& Sinks() {
	static std::vector<EventSink*> instance;
	return instance;
}

}  // namespace

void RegisterEventSink(EventSink* sink) {
	if (!sink) {
		return;
	}
	auto& sinks = Sinks();
	if (std::find(sinks.begin(), sinks.end(), sink) == sinks.end()) {
		sinks.push_back(sink);
	}
}

void UnregisterEventSink(EventSink* sink) {
	auto& sinks = Sinks();
	const auto it = std::find(sinks.begin(), sinks.end(), sink);
	if (it != sinks.end()) {
		sinks.erase(it);
	}
}

bool HasAnyEventSink() {
	return !Sinks().empty();
}

void EmitToAllSinks(const Event& ev) {
	for (auto* sink : Sinks()) {
		sink->Emit(ev);
	}
}

void FlushAllSinks() {
	for (auto* sink : Sinks()) {
		sink->Flush();
	}
}

std::string EngineStringToUtf8(const std::string& koi8r) {
	// Движок держит текст в UTF-8, потребитель тоже ждёт UTF-8 -- границы здесь больше нет.
	// Перекодировка, оставшаяся с байтовых времён, теперь разбирала бы готовый UTF-8 как
	// KOI8-R и удваивала каждую букву (issue #3681).
	return koi8r;
}

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
