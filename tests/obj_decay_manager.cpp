#include "gameplay/core/obj_decay_manager.h"
#include "engine/entities/obj_data.h"

#include <gtest/gtest.h>

class ObjDecayManagerTest : public ::testing::Test {
 protected:
	void SetUp() override {
		objs_.clear();
	}

	void TearDown() override {
		for (auto &obj : objs_) {
			obj_decay_manager.remove(obj.get());
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
	auto before = obj_decay_manager.size();

	obj_decay_manager.insert(obj.get());
	EXPECT_EQ(obj_decay_manager.size(), before + 1);
	EXPECT_TRUE(obj_decay_manager.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, RemoveCleansUp) {
	auto obj = make_obj(100);
	obj_decay_manager.insert(obj.get());
	obj_decay_manager.add_env_check(obj.get());
	obj_decay_manager.add_timed_spell_obj(obj.get());

	auto ts_before = obj_decay_manager.timed_spell_size();
	obj_decay_manager.remove(obj.get());
	EXPECT_FALSE(obj_decay_manager.contains(obj.get()));
	EXPECT_EQ(obj_decay_manager.timed_spell_size(), ts_before - 1);
}

TEST_F(ObjDecayManagerTest, ProcessTick_NoExpiry) {
	auto obj = make_obj(100);
	obj_decay_manager.insert(obj.get());

	auto result = obj_decay_manager.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
	EXPECT_TRUE(result.env_destroy.empty());
	EXPECT_TRUE(obj_decay_manager.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, ProcessTick_ExpiresAfterTimer) {
	auto obj = make_obj(3);
	obj_decay_manager.insert(obj.get());

	obj_decay_manager.process_tick();  // tick 1
	EXPECT_TRUE(obj_decay_manager.contains(obj.get()));

	obj_decay_manager.process_tick();  // tick 2
	EXPECT_TRUE(obj_decay_manager.contains(obj.get()));

	auto result = obj_decay_manager.process_tick();  // tick 3 - expires
	EXPECT_EQ(result.decay_timer.size(), 1u);
	EXPECT_EQ(result.decay_timer.front(), obj.get());
	EXPECT_FALSE(obj_decay_manager.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, MultipleObjects_ExpireInOrder) {
	auto obj1 = make_obj(1);
	auto obj2 = make_obj(3);
	auto obj3 = make_obj(2);

	obj_decay_manager.insert(obj1.get());
	obj_decay_manager.insert(obj2.get());
	obj_decay_manager.insert(obj3.get());

	auto r1 = obj_decay_manager.process_tick();
	EXPECT_EQ(r1.decay_timer.size(), 1u);
	EXPECT_EQ(r1.decay_timer.front(), obj1.get());

	auto r2 = obj_decay_manager.process_tick();
	EXPECT_EQ(r2.decay_timer.size(), 1u);
	EXPECT_EQ(r2.decay_timer.front(), obj3.get());

	auto r3 = obj_decay_manager.process_tick();
	EXPECT_EQ(r3.decay_timer.size(), 1u);
	EXPECT_EQ(r3.decay_timer.front(), obj2.get());
}

TEST_F(ObjDecayManagerTest, Remove_PreventsExpiry) {
	auto obj = make_obj(2);
	obj_decay_manager.insert(obj.get());
	obj_decay_manager.remove(obj.get());

	obj_decay_manager.process_tick();
	auto result = obj_decay_manager.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_ShortenDeadline) {
	auto obj = make_obj(100);
	obj_decay_manager.insert(obj.get());

	static_cast<CObjectPrototype &>(*obj).set_timer(2);
	obj_decay_manager.on_timer_changed(obj.get());

	obj_decay_manager.process_tick();
	auto result = obj_decay_manager.process_tick();
	EXPECT_EQ(result.decay_timer.size(), 1u);
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_ExtendDeadline) {
	auto obj = make_obj(2);
	obj_decay_manager.insert(obj.get());

	static_cast<CObjectPrototype &>(*obj).set_timer(50);
	obj_decay_manager.on_timer_changed(obj.get());

	obj_decay_manager.process_tick();
	auto result = obj_decay_manager.process_tick();
	EXPECT_TRUE(result.decay_timer.empty());
	EXPECT_TRUE(obj_decay_manager.contains(obj.get()));
}

TEST_F(ObjDecayManagerTest, OnTimerChanged_UntrackedIsNoop) {
	auto obj = make_obj(100);
	auto before = obj_decay_manager.size();
	obj_decay_manager.on_timer_changed(obj.get());
	EXPECT_EQ(obj_decay_manager.size(), before);
}

TEST_F(ObjDecayManagerTest, InsertDuplicate_UpdatesDeadline) {
	auto obj = make_obj(10);
	obj_decay_manager.insert(obj.get());
	auto size_after_first = obj_decay_manager.size();

	static_cast<CObjectPrototype &>(*obj).set_timer(2);
	obj_decay_manager.insert(obj.get());  // second insert = update
	EXPECT_EQ(obj_decay_manager.size(), size_after_first);

	obj_decay_manager.process_tick();
	auto result = obj_decay_manager.process_tick();
	EXPECT_EQ(result.decay_timer.size(), 1u);
}

TEST_F(ObjDecayManagerTest, TimedSpellTracking) {
	auto obj = make_obj(100);
	auto before = obj_decay_manager.timed_spell_size();

	obj_decay_manager.add_timed_spell_obj(obj.get());
	EXPECT_EQ(obj_decay_manager.timed_spell_size(), before + 1);

	obj_decay_manager.remove_timed_spell_obj(obj.get());
	EXPECT_EQ(obj_decay_manager.timed_spell_size(), before);
}

TEST_F(ObjDecayManagerTest, GetDeadline_Tracked) {
	auto obj = make_obj(5);
	obj_decay_manager.insert(obj.get());

	auto now = obj_decay_manager.current_mud_hour();
	auto dl = obj_decay_manager.get_deadline(obj.get());
	EXPECT_EQ(dl, now + 5);
}

TEST_F(ObjDecayManagerTest, GetDeadline_Untracked) {
	auto obj = make_obj(5);
	auto dl = obj_decay_manager.get_deadline(obj.get());
	EXPECT_EQ(dl, UINT64_MAX);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
