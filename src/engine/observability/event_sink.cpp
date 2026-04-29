#include "event_sink.h"

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

}  // namespace observability

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
