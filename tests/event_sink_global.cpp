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

TEST(EventSinkRegistry, EmptyByDefaultHasAnyReportsFalse) {
	EXPECT_FALSE(observability::HasAnyEventSink());
	observability::Event e;
	e.name = "noop_check";
	observability::EmitToAllSinks(e);  // must not crash with no sinks
	observability::FlushAllSinks();
	SUCCEED();
}

TEST(EventSinkRegistry, RegisteredSinkReceivesAllEmits) {
	CountingSink fake;
	observability::RegisterEventSink(&fake);
	EXPECT_TRUE(observability::HasAnyEventSink());
	observability::Event e;
	e.name = "ping";
	observability::EmitToAllSinks(e);
	observability::EmitToAllSinks(e);
	observability::FlushAllSinks();
	EXPECT_EQ(fake.emits, 2);
	EXPECT_EQ(fake.flushes, 1);
	observability::UnregisterEventSink(&fake);
	EXPECT_FALSE(observability::HasAnyEventSink());
}

TEST(EventSinkRegistry, UnregisteredSinkStopsReceiving) {
	CountingSink fake;
	observability::RegisterEventSink(&fake);
	observability::UnregisterEventSink(&fake);
	observability::Event e;
	e.name = "after_reset";
	observability::EmitToAllSinks(e);
	EXPECT_EQ(fake.emits, 0);
}

TEST(EventSinkRegistry, MultipleSinksAllReceive) {
	CountingSink a;
	CountingSink b;
	observability::RegisterEventSink(&a);
	observability::RegisterEventSink(&b);
	observability::Event e;
	e.name = "fanout";
	observability::EmitToAllSinks(e);
	EXPECT_EQ(a.emits, 1);
	EXPECT_EQ(b.emits, 1);
	observability::UnregisterEventSink(&a);
	observability::UnregisterEventSink(&b);
}

TEST(EventSinkRegistry, DoubleRegistrationIsIdempotent) {
	CountingSink fake;
	observability::RegisterEventSink(&fake);
	observability::RegisterEventSink(&fake);
	observability::Event e;
	e.name = "dup";
	observability::EmitToAllSinks(e);
	EXPECT_EQ(fake.emits, 1);  // no double delivery
	observability::UnregisterEventSink(&fake);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
