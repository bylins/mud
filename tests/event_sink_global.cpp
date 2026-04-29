#include <gtest/gtest.h>

#include "engine/observability/event_sink.h"

namespace {

class CountingSink : public observability::EventSink {
public:
	int emits = 0;
	int flushes = 0;
	void Emit(const observability::Event&) override { ++emits; }
	void Flush() override { ++flushes; }
};

}  // namespace

TEST(GlobalEventSink, NoOpByDefaultDoesNotCrash) {
	// No SetGlobalEventSink call; default must be a usable NoOp.
	observability::Event e;
	e.name = "noop_check";
	observability::GlobalEventSink().Emit(e);
	observability::GlobalEventSink().Flush();
	SUCCEED();
}

TEST(GlobalEventSink, SetterRoutesEventsToInstalledSink) {
	CountingSink fake;
	observability::SetGlobalEventSink(&fake);
	observability::Event e;
	e.name = "ping";
	observability::GlobalEventSink().Emit(e);
	observability::GlobalEventSink().Emit(e);
	observability::GlobalEventSink().Flush();
	EXPECT_EQ(fake.emits, 2);
	EXPECT_EQ(fake.flushes, 1);
	// Reset for other tests.
	observability::SetGlobalEventSink(nullptr);
}

TEST(GlobalEventSink, ResetToNullptrFallsBackToNoOp) {
	CountingSink fake;
	observability::SetGlobalEventSink(&fake);
	observability::SetGlobalEventSink(nullptr);
	observability::Event e;
	e.name = "after_reset";
	observability::GlobalEventSink().Emit(e);
	EXPECT_EQ(fake.emits, 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
