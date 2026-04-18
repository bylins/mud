#include "engine/db/world_objects.h"
#include "engine/db/db.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/entities/zone.h"
#include "engine/entities/char_data.h"

#include <gtest/gtest.h>

#define decay_mgr world_objects.decay_manager()

class ObjDecayManagerTest : public ::testing::Test {
 protected:
	void SetUp() override {
		objs_.clear();
	}

	void TearDown() override {
		for (auto &obj : objs_) {
			decay_mgr.remove(obj.get());
		}
		objs_.clear();
	}

	ObjData::shared_ptr make_obj(int timer) {
		auto proto = std::make_shared<CObjectPrototype>(static_cast<ObjVnum>(900000 + objs_.size()));
		auto obj = std::make_shared<ObjData>(*proto);
		// Use base class set_timer to avoid virtual dispatch to ObjData::set_timer
		// which would call on_timer_changed on the global manager
		static_cast<CObjectPrototype &>(*obj).set_timer(timer);
		// kTicktimer enables timer countdown in ObjDecayManager. Without it
		// the deadline is frozen at UINT64_MAX (room-object semantics).
		// These low-level tests cover the timer-tick mechanism, so they need
		// the flag set to mirror the "carried by player" state.
		obj->set_extra_flag(EObjFlag::kTicktimer);
		objs_.push_back(obj);
		protos_.push_back(proto);
		return obj;
	}

	std::vector<ObjData::shared_ptr> objs_;
	std::vector<std::shared_ptr<CObjectPrototype>> protos_;
};

TEST_F(ObjDecayManagerTest, InsertAndSize) {
	auto obj = make_obj(100);
	auto before = decay_mgr.size();

	decay_mgr.insert(obj.get());
	EXPECT_EQ(decay_mgr.size(), before + 1);
	EXPECT_TRUE(decay_mgr.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, RemoveCleansUp) {
	auto obj = make_obj(100);
	decay_mgr.insert(obj.get());
	decay_mgr.add_env_check(obj.get());
	decay_mgr.add_timed_spell_obj(obj.get());

	auto ts_before = decay_mgr.timed_spell_size();
	decay_mgr.remove(obj.get());
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
	EXPECT_EQ(decay_mgr.timed_spell_size(), ts_before - 1);
}

TEST_F(ObjDecayManagerTest, ProcessTick_NoExpiry) {
	auto obj = make_obj(100);
	decay_mgr.insert(obj.get());

	auto result = decay_mgr.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
	EXPECT_TRUE(result.env_destroy.empty());
	EXPECT_TRUE(decay_mgr.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, ProcessTick_ExpiresAfterTimer) {
	auto obj = make_obj(3);
	decay_mgr.insert(obj.get());

	decay_mgr.process_tick();  // tick 1
	EXPECT_TRUE(decay_mgr.contains(obj.get()));

	decay_mgr.process_tick();  // tick 2
	EXPECT_TRUE(decay_mgr.contains(obj.get()));

	auto result = decay_mgr.process_tick();  // tick 3 - expires
	EXPECT_EQ(result.decay_timer.size(), 1u);
	EXPECT_EQ(result.decay_timer.front(), obj.get());
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, MultipleObjects_ExpireInOrder) {
	auto obj1 = make_obj(1);
	auto obj2 = make_obj(3);
	auto obj3 = make_obj(2);

	decay_mgr.insert(obj1.get());
	decay_mgr.insert(obj2.get());
	decay_mgr.insert(obj3.get());

	auto r1 = decay_mgr.process_tick();
	EXPECT_EQ(r1.decay_timer.size(), 1u);
	EXPECT_EQ(r1.decay_timer.front(), obj1.get());

	auto r2 = decay_mgr.process_tick();
	EXPECT_EQ(r2.decay_timer.size(), 1u);
	EXPECT_EQ(r2.decay_timer.front(), obj3.get());

	auto r3 = decay_mgr.process_tick();
	EXPECT_EQ(r3.decay_timer.size(), 1u);
	EXPECT_EQ(r3.decay_timer.front(), obj2.get());
}

TEST_F(ObjDecayManagerTest, Remove_PreventsExpiry) {
	auto obj = make_obj(2);
	decay_mgr.insert(obj.get());
	decay_mgr.remove(obj.get());

	decay_mgr.process_tick();
	auto result = decay_mgr.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_ShortenDeadline) {
	auto obj = make_obj(100);
	decay_mgr.insert(obj.get());

	static_cast<CObjectPrototype &>(*obj).set_timer(2);
	decay_mgr.on_timer_changed(obj.get());

	decay_mgr.process_tick();
	auto result = decay_mgr.process_tick();
	EXPECT_EQ(result.decay_timer.size(), 1u);
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_ExtendDeadline) {
	auto obj = make_obj(2);
	decay_mgr.insert(obj.get());

	static_cast<CObjectPrototype &>(*obj).set_timer(50);
	decay_mgr.on_timer_changed(obj.get());

	decay_mgr.process_tick();
	auto result = decay_mgr.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
	EXPECT_TRUE(decay_mgr.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_UntrackedIsNoop) {
	auto obj = make_obj(100);
	auto before = decay_mgr.size();
	decay_mgr.on_timer_changed(obj.get());
	EXPECT_EQ(decay_mgr.size(), before);
}

TEST_F(ObjDecayManagerTest, InsertDuplicate_UpdatesDeadline) {
	auto obj = make_obj(10);
	decay_mgr.insert(obj.get());
	auto size_after_first = decay_mgr.size();

	static_cast<CObjectPrototype &>(*obj).set_timer(2);
	decay_mgr.insert(obj.get());  // second insert = update
	EXPECT_EQ(decay_mgr.size(), size_after_first);

	decay_mgr.process_tick();
	auto result = decay_mgr.process_tick();
	EXPECT_EQ(result.decay_timer.size(), 1u);
}

TEST_F(ObjDecayManagerTest, TimedSpellTracking) {
	auto obj = make_obj(100);
	auto before = decay_mgr.timed_spell_size();

	decay_mgr.add_timed_spell_obj(obj.get());
	EXPECT_EQ(decay_mgr.timed_spell_size(), before + 1);

	decay_mgr.remove_timed_spell_obj(obj.get());
	EXPECT_EQ(decay_mgr.timed_spell_size(), before);
}

TEST_F(ObjDecayManagerTest, GetDeadline_Tracked) {
	auto obj = make_obj(5);
	decay_mgr.insert(obj.get());

	auto now = decay_mgr.current_mud_hour();
	auto dl = decay_mgr.get_deadline(obj.get());
	EXPECT_EQ(dl, now + 5);
}

TEST_F(ObjDecayManagerTest, GetDeadline_Untracked) {
	auto obj = make_obj(5);
	auto dl = decay_mgr.get_deadline(obj.get());
	EXPECT_EQ(dl, UINT64_MAX);
}

// ---------------------------------------------------------------------------
// Regression tests for GitHub issue #3178:
//   - kZonedecay objects must "crumble" only when located outside their
//     home zone, regardless of where they are (in a room, carried, worn,
//     inside a container).
//   - Objects without the kTicktimer flag must keep their decay timer
//     frozen (they "live" only when a player has interacted with them).
//
// The tests below set up a minimal world (two rooms in two distinct zones)
// so that ObjDecayManager::process_tick() can resolve up_obj_where() and
// the zone vnums of objects.
// ---------------------------------------------------------------------------

namespace {

constexpr ZoneVnum kHomeZoneVnum = 999001;
constexpr ZoneVnum kForeignZoneVnum = 999002;

class ZonedecayTest : public ::testing::Test {
 protected:
	void SetUp() override {
		objs_.clear();
		chars_.clear();
		saved_world_size_ = world.size();
		saved_zone_table_size_ = zone_table.size();

		ZoneData home_zone;
		home_zone.vnum = kHomeZoneVnum;
		home_zone.lifespan = 60;
		zone_table.push_back(std::move(home_zone));
		home_zone_rn_ = static_cast<ZoneRnum>(zone_table.size() - 1);

		ZoneData foreign_zone;
		foreign_zone.vnum = kForeignZoneVnum;
		foreign_zone.lifespan = 60;
		zone_table.push_back(std::move(foreign_zone));
		foreign_zone_rn_ = static_cast<ZoneRnum>(zone_table.size() - 1);

		auto *home_room = new RoomData();
		home_room->vnum = kHomeZoneVnum * 100;
		home_room->zone_rn = home_zone_rn_;
		world.push_back(home_room);
		home_room_rn_ = static_cast<RoomRnum>(world.size() - 1);

		auto *foreign_room = new RoomData();
		foreign_room->vnum = kForeignZoneVnum * 100;
		foreign_room->zone_rn = foreign_zone_rn_;
		world.push_back(foreign_room);
		foreign_room_rn_ = static_cast<RoomRnum>(world.size() - 1);
	}

	void TearDown() override {
		for (auto &obj : objs_) {
			decay_mgr.remove(obj.get());
		}
		objs_.clear();
		chars_.clear();

		while (world.size() > saved_world_size_) {
			delete world.back();
			world.pop_back();
		}
		while (zone_table.size() > saved_zone_table_size_) {
			zone_table.pop_back();
		}
	}

	// Build a fresh ObjData. Use base-class set_timer so the call does not
	// touch the global decay manager during construction.
	ObjData::shared_ptr make_obj(int timer, ZoneVnum zone_from, bool zonedecay) {
		auto proto = std::make_shared<CObjectPrototype>(
			static_cast<ObjVnum>(910000 + objs_.size()));
		auto obj = std::make_shared<ObjData>(*proto);
		static_cast<CObjectPrototype &>(*obj).set_timer(timer);
		obj->set_vnum_zone_from(zone_from);
		if (zonedecay) {
			obj->set_extra_flag(EObjFlag::kZonedecay);
		}
		objs_.push_back(obj);
		protos_.push_back(proto);
		return obj;
	}

	CharData::shared_ptr make_char_in(RoomRnum room) {
		auto ch = std::make_shared<CharData>();
		ch->in_room = room;
		chars_.push_back(ch);
		return ch;
	}

	std::vector<ObjData::shared_ptr> objs_;
	std::vector<std::shared_ptr<CObjectPrototype>> protos_;
	std::vector<CharData::shared_ptr> chars_;

	std::size_t saved_world_size_ = 0;
	std::size_t saved_zone_table_size_ = 0;
	ZoneRnum home_zone_rn_ = 0;
	ZoneRnum foreign_zone_rn_ = 0;
	RoomRnum home_room_rn_ = 0;
	RoomRnum foreign_room_rn_ = 0;
};

// kZonedecay timer<=0 lying in its home-zone room must never expire from
// section 1 (timer queue) or section 2 (env check).
TEST_F(ZonedecayTest, NoTimer_HomeRoom_SurvivesIndefinitely) {
	auto obj = make_obj(/*timer=*/0, kHomeZoneVnum, /*zonedecay=*/true);
	obj->set_in_room(home_room_rn_);

	decay_mgr.insert(obj.get());
	decay_mgr.add_env_check(obj.get());

	for (int i = 0; i < 50; ++i) {
		auto r = decay_mgr.process_tick();
		ASSERT_TRUE(r.decay_timer.empty()) << "tick " << i;
		ASSERT_TRUE(r.env_destroy.empty()) << "tick " << i;
		ASSERT_TRUE(decay_mgr.contains(obj.get())) << "tick " << i;
	}
}

// kZonedecay timer<=0 in a room of a foreign zone must decay via section 2.
TEST_F(ZonedecayTest, NoTimer_ForeignRoom_DecaysOnNextTick) {
	auto obj = make_obj(/*timer=*/0, kHomeZoneVnum, /*zonedecay=*/true);
	obj->set_in_room(foreign_room_rn_);

	decay_mgr.insert(obj.get());
	decay_mgr.add_env_check(obj.get());

	auto r = decay_mgr.process_tick();
	const bool destroyed = !r.env_destroy.empty() || !r.decay_timer.empty();
	EXPECT_TRUE(destroyed);
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
}

// Regression for #3178: kZonedecay carried by a character standing in a
// foreign zone must decay even though the object is not in m_env_check_objs.
TEST_F(ZonedecayTest, NoTimer_CarriedInForeignZone_Decays) {
	auto ch = make_char_in(foreign_room_rn_);
	auto obj = make_obj(/*timer=*/0, kHomeZoneVnum, /*zonedecay=*/true);
	obj->set_carried_by(ch.get());
	obj->set_in_room(kNowhere);

	decay_mgr.insert(obj.get());
	// Intentionally NOT calling add_env_check: a carried object is not
	// considered to be in a room.

	auto r = decay_mgr.process_tick();
	const bool destroyed = !r.env_destroy.empty() || !r.decay_timer.empty();
	EXPECT_TRUE(destroyed) << "kZonedecay item carried into a foreign zone "
							  "must be scheduled for destruction";
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
}

// kZonedecay worn by a character in a foreign zone must also decay.
TEST_F(ZonedecayTest, NoTimer_WornInForeignZone_Decays) {
	auto ch = make_char_in(foreign_room_rn_);
	auto obj = make_obj(/*timer=*/0, kHomeZoneVnum, /*zonedecay=*/true);
	obj->set_worn_by(ch.get());
	obj->set_in_room(kNowhere);

	decay_mgr.insert(obj.get());

	auto r = decay_mgr.process_tick();
	const bool destroyed = !r.env_destroy.empty() || !r.decay_timer.empty();
	EXPECT_TRUE(destroyed);
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
}

// kZonedecay carried by a character standing in its home zone must NOT
// decay even after many ticks.
TEST_F(ZonedecayTest, NoTimer_CarriedInHomeZone_Survives) {
	auto ch = make_char_in(home_room_rn_);
	auto obj = make_obj(/*timer=*/0, kHomeZoneVnum, /*zonedecay=*/true);
	obj->set_carried_by(ch.get());
	obj->set_in_room(kNowhere);

	decay_mgr.insert(obj.get());

	for (int i = 0; i < 20; ++i) {
		auto r = decay_mgr.process_tick();
		ASSERT_TRUE(r.decay_timer.empty()) << "tick " << i;
		ASSERT_TRUE(r.env_destroy.empty()) << "tick " << i;
		ASSERT_TRUE(decay_mgr.contains(obj.get())) << "tick " << i;
	}
}

// ---------------------------------------------------------------------------
// kTicktimer freeze: objects without the kTicktimer flag must have their
// decay timer frozen (they only start "ageing" after a player picks them up
// and the kTicktimer flag is set). The pre-#3065 behaviour froze room
// objects via NO_TIMER(); the new ObjDecayManager lost this.
// ---------------------------------------------------------------------------

// Object with positive timer placed in a room without kTicktimer flag must
// not expire Б─■ its timer is frozen until a player interacts with it.
TEST_F(ZonedecayTest, NoTicktimer_RoomObject_TimerFrozen) {
	auto obj = make_obj(/*timer=*/5, kHomeZoneVnum, /*zonedecay=*/false);
	obj->set_in_room(home_room_rn_);
	ASSERT_FALSE(obj->has_flag(EObjFlag::kTicktimer));

	decay_mgr.insert(obj.get());
	decay_mgr.add_env_check(obj.get());

	for (int i = 0; i < 30; ++i) {
		auto r = decay_mgr.process_tick();
		ASSERT_TRUE(r.decay_timer.empty()) << "tick " << i;
		ASSERT_TRUE(decay_mgr.contains(obj.get())) << "tick " << i;
	}
}

// Once the kTicktimer flag is set and the manager is notified, the timer
// must start counting down and the object must eventually expire.
TEST_F(ZonedecayTest, TicktimerSet_TimerStartsCountingDown) {
	auto obj = make_obj(/*timer=*/3, kHomeZoneVnum, /*zonedecay=*/false);
	obj->set_in_room(home_room_rn_);

	decay_mgr.insert(obj.get());

	// Two ticks in the frozen state Б─■ must not expire yet.
	for (int i = 0; i < 2; ++i) {
		auto r = decay_mgr.process_tick();
		ASSERT_TRUE(r.decay_timer.empty());
		ASSERT_TRUE(decay_mgr.contains(obj.get()));
	}

	// Player picks the object up: kTicktimer flag is set and the manager
	// is notified to recompute the deadline.
	obj->set_extra_flag(EObjFlag::kTicktimer);
	decay_mgr.on_timer_changed(obj.get());

	// Three more ticks Б─■ at most three are needed to drain the timer.
	bool expired = false;
	for (int i = 0; i < 4; ++i) {
		auto r = decay_mgr.process_tick();
		if (!r.decay_timer.empty() || !r.env_destroy.empty()) {
			expired = true;
			break;
		}
	}
	EXPECT_TRUE(expired) << "object with kTicktimer set must expire";
	EXPECT_FALSE(decay_mgr.contains(obj.get()));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
