#include "engine/db/world_objects.h"
#include "engine/entities/obj_data.h"

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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
