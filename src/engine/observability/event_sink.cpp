#include "event_sink.h"

#include "utils/utils.h"

#include <array>

namespace observability {

namespace {

class NoOpEventSink : public EventSink {
public:
	void Emit(const Event&) override {}
	void Flush() override {}
};

NoOpEventSink& NoOpInstance() {
	static NoOpEventSink instance;
	return instance;
}

EventSink* g_sink = nullptr;

}  // namespace

EventSink& GlobalEventSink() {
	return g_sink ? *g_sink : NoOpInstance();
}

void SetGlobalEventSink(EventSink* sink) {
	g_sink = sink;
}

std::string EngineStringToUtf8(const std::string& koi8r) {
	std::array<char, 4096> buf{};
	std::string mut = koi8r;  // koi_to_utf8 takes char*, not const
	mut.push_back('\0');
	koi_to_utf8(mut.data(), buf.data());
	return std::string(buf.data());
}

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
