#include <gtest/gtest.h>

#if defined(WITH_LUAJIT_PROTOTYPE)

#include "engine/db/db.h"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/scripting/dg_event.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/scripting/lua/lua_script_engine.h"
#include "engine/scripting/lua/lua_trigger_exec.h"

#include <memory>
#include <string>
#include <vector>

namespace {

class LuaTriggerExecTest : public ::testing::Test {
 protected:
	static constexpr int kIndexCapacity = 32;

	void SetUp() override {
		trigger_list = GlobalTriggersStorage();
		trig_index = new IndexData *[kIndexCapacity]{};
		top_of_trigt = 0;

		saved_mob_index_ = mob_index;
		saved_mob_proto_ = mob_proto;
		saved_top_of_mobt_ = top_of_mobt;
		mob_index_storage_.emplace_back(910001);
		mob_index_storage_[0].total_online = 1;
		mob_proto_storage_ = std::make_unique<CharData[]>(1);
		mob_proto_storage_[0].SetNpcAttribute(true);
		mob_proto_storage_[0].player_specials = player_special_data::s_for_mobiles;
		mob_proto_storage_[0].set_rnum(0);
		mob_index = mob_index_storage_.data();
		mob_proto = mob_proto_storage_.get();
		top_of_mobt = 0;

		owner_ = std::make_shared<CharData>();
		owner_->SetNpcAttribute(true);
		owner_->player_specials = player_special_data::s_for_mobiles;
		owner_->set_rnum(0);
		owner_->SetFlag(EMobFlag::kNpc);
		owner_->set_uid(910001);
		character_list.push_front(owner_);
		ClearWorldVars();
	}

	void TearDown() override {
		if (object_ && world_objects.get_by_raw_ptr(object_.get())) {
			world_objects.AddToExtractedList(object_.get());
			world_objects.PurgeExtractedList();
		}
		object_.reset();
		world_objects.purge();
		lua_scripting::LuaScriptEngine::HeartbeatCleanup();
		ClearWorldVars();

		if (room_) {
			ASSERT_EQ(world.back(), room_);
			world.pop_back();
			delete room_;
			room_ = nullptr;
			if (dummy_room_) {
				ASSERT_EQ(world.back(), dummy_room_);
				world.pop_back();
				delete dummy_room_;
				dummy_room_ = nullptr;
			}
			top_of_world = world.empty() ? kNowhere : static_cast<RoomRnum>(world.size() - 1);
		}
		character_list.remove(owner_);
		owner_.reset();
		character_list.purge();
		mob_proto_storage_.reset();
		mob_index = saved_mob_index_;
		mob_proto = saved_mob_proto_;
		top_of_mobt = saved_top_of_mobt_;

		trigger_list = GlobalTriggersStorage();
		for (int i = 0; i < top_of_trigt; ++i) {
			delete trig_index[i]->proto;
			free(trig_index[i]);
		}
		delete[] trig_index;
		trig_index = nullptr;
		top_of_trigt = 0;
	}

	int AddPrototype(int vnum,
		const std::string &source,
		int attach_type = MOB_TRIGGER,
		TriggerScriptLanguage language = TriggerScriptLanguage::Lua) {
		auto *prototype = new Trigger(top_of_trigt, "lua exec test", attach_type, 0);
		prototype->set_script_language(language);
		prototype->set_lua_script_source(source);
		AddTrigIndexEntry(vnum, prototype);
		return top_of_trigt - 1;
	}

	Trigger *Attach(Script *script, int rnum) {
		auto *trigger = read_trigger(rnum);
		EXPECT_TRUE(add_trigger(script, trigger, -1));
		return trigger;
	}

	static size_t CountTriggers(TriggersList &triggers) {
		size_t count = 0;
		for (auto *trigger : triggers) {
			(void) trigger;
			++count;
		}
		return count;
	}

	int Run(int rnum, const lua_scripting::LuaTriggerContext &context) {
		return lua_scripting::LuaScriptEngine::RunTrigger(trig_index[rnum]->proto, context);
	}

	int Eval(const std::string &expression) {
		const int rnum = AddPrototype(990000 + top_of_trigt, "return tonumber(" + expression + ")");
		lua_scripting::LuaTriggerContext context;
		context.trigger = trig_index[rnum]->proto;
		context.owner = owner_.get();
		context.trigger_type = MOB_TRIGGER;
		return Run(rnum, context);
	}

	void ClearWorldVars() {
		const int rnum = AddPrototype(989000 + top_of_trigt, R"(
			mud.world.clearContext(77)
			mud.world.clearContext(78)
			mud.world.clearContext(79)
			mud.world.clearContext(80)
			return true
		)");
		lua_scripting::LuaTriggerContext context;
		context.trigger = trig_index[rnum]->proto;
		context.owner = owner_.get();
		context.trigger_type = MOB_TRIGGER;
		EXPECT_EQ(Run(rnum, context), 1);
	}

	ObjData *MakeObject() {
		constexpr ObjVnum test_vnum = 920001;
		auto rnum = obj_proto.get_rnum(test_vnum);
		if (rnum == kNothing) {
			rnum = static_cast<ObjRnum>(obj_proto.add(
				std::make_shared<CObjectPrototype>(test_vnum), test_vnum));
		}
		object_ = world_objects.create_raw_from_prototype_by_rnum(rnum);
		EXPECT_TRUE(object_);
		object_id_ = object_->get_id();
		return object_.get();
	}

	RoomData *MakeRoom() {
		if (world.empty()) {
			dummy_room_ = new RoomData();
			dummy_room_->vnum = kNowhere;
			world.push_back(dummy_room_);
		}
		room_ = new RoomData();
		room_->vnum = 930001;
		room_->script = std::make_shared<Script>();
		world.push_back(room_);
		top_of_world = static_cast<RoomRnum>(world.size() - 1);
		return room_;
	}

	std::shared_ptr<CharData> owner_;
	ObjData::shared_ptr object_;
	object_id_t object_id_ = 0;
	RoomData *room_ = nullptr;
	RoomData *dummy_room_ = nullptr;
	IndexData *saved_mob_index_ = nullptr;
	CharData *saved_mob_proto_ = nullptr;
	MobRnum saved_top_of_mobt_ = 0;
	std::vector<IndexData> mob_index_storage_;
	std::unique_ptr<CharData[]> mob_proto_storage_;
};

TEST_F(LuaTriggerExecTest, MobExecInheritsContextAndChildFalseDoesNotStopParent) {
	auto *object = MakeObject();
	const int child = AddPrototype(7001, std::string(R"(
		local ok = ctx.owner.uid == 910001
			and ctx.actor.uid == 910002 and ctx.victim.uid == 910003
			and ctx.object.id == )") + std::to_string(object_id_) + R"(
			and ctx.command == "say" and ctx.argument == "hello"
		mud.world.set(77, "inherited", ok and 41 or 13)
		return false
	)");
	Attach(owner_->script.get(), child);

	auto actor = std::make_shared<CharData>();
	actor->set_uid(910002);
	character_list.push_front(actor);
	auto victim = std::make_shared<CharData>();
	victim->set_uid(910003);
	character_list.push_front(victim);
	const int parent = AddPrototype(7000, R"(
		if not mud.exec(7001, ctx.owner) then return 9 end
		return 42
	)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.actor = actor.get();
	context.victim = victim.get();
	context.object = object;
	context.command = "say";
	context.argument = "hello";
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 42);
	EXPECT_EQ(Eval("mud.world.get(77, 'inherited', 0)"), 41);
	EXPECT_EQ(owner_->script->script_trig_list.find_by_vnum(7001)->is_runned, false);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 1u);

	character_list.remove(actor);
	character_list.remove(victim);
}

TEST_F(LuaTriggerExecTest, ObjectAndRoomTargetsBecomeChildOwner) {
	auto *object = MakeObject();
	auto *room = MakeRoom();
	const int object_child = AddPrototype(7101,
		"mud.world.set(78, 'object_owner', ctx.owner.id); return true", OBJ_TRIGGER);
	const int room_child = AddPrototype(7102,
		"mud.world.set(78, 'room_owner', ctx.owner.vnum); return true", WLD_TRIGGER);
	Attach(object->get_script().get(), object_child);
	Attach(room->script.get(), room_child);

	const int parent = AddPrototype(7100,
		"local a = mud.exec(7101, mud.objById(" + std::to_string(object_id_) + "))\n"
		"local b = mud.exec(7102, mud.room(930001))\n"
		"return a and b and 1 or 0");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 1);
	EXPECT_EQ(Eval("mud.world.get(78, 'object_owner', 0)"), object_id_);
	EXPECT_EQ(Eval("mud.world.get(78, 'room_owner', 0)"), 930001);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(object_child).size(), 1u);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(room_child).size(), 1u);
}

TEST_F(LuaTriggerExecTest, RejectsMissingDetachedDgAndInvalidTargetsWithoutClones) {
	const int detached = AddPrototype(7201, "return true");
	const int dg = AddPrototype(7202, "", MOB_TRIGGER, TriggerScriptLanguage::Dg);
	const int lua = AddPrototype(7203, "return true");
	Attach(owner_->script.get(), dg);
	Attach(owner_->script.get(), lua);
	const auto detached_online = trig_index[detached]->total_online;
	const auto dg_online = trig_index[dg]->total_online;

	const int parent = AddPrototype(7200, R"(
		local failures = 0
		if not mud.exec(999999, ctx.owner) then failures = failures + 1 end
		if not mud.exec(7201, ctx.owner) then failures = failures + 1 end
		if not mud.exec(7202, ctx.owner) then failures = failures + 1 end
		if not mud.exec(7202, {}) then failures = failures + 1 end
		if not mud.exec(0, ctx.owner) then failures = failures + 1 end
		return failures
	)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 5);
	EXPECT_EQ(trig_index[detached]->total_online, detached_online);
	EXPECT_EQ(trig_index[dg]->total_online, dg_online);

	owner_->set_purged(true);
	const int stale_parent = AddPrototype(7204,
		"return mud.exec(7203, ctx.owner) and 0 or 1");
	context.trigger = trig_index[stale_parent]->proto;
	EXPECT_EQ(Run(stale_parent, context), 1);
	EXPECT_EQ(trig_index[lua]->total_online, 1);
	owner_->set_purged(false);
}

TEST_F(LuaTriggerExecTest, WaitingCloneLivesUntilResumeThenIsRemoved) {
	const int child = AddPrototype(7301, R"(
		mud.world.set(79, 'phase', 1)
		mud.wait(1)
		mud.world.set(79, 'phase', 2)
		return true
	)");
	Attach(owner_->script.get(), child);
	const int parent = AddPrototype(7300, "return mud.exec(7301, ctx.owner)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 1);
	EXPECT_EQ(Eval("mud.world.get(79, 'phase', 0)"), 1);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 2u);

	process_events();
	process_events();
	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_EQ(Eval("mud.world.get(79, 'phase', 0)"), 2);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 1u);
}

TEST_F(LuaTriggerExecTest, OwnerPurgeCancelsWaitingCloneSafely) {
	const int child = AddPrototype(7401, "mud.wait(10); return true");
	Attach(owner_->script.get(), child);
	const int parent = AddPrototype(7400, "return mud.exec(7401, ctx.owner)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	ASSERT_EQ(Run(parent, context), 1);
	ASSERT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 2u);

	character_list.AddToExtractedList(owner_.get());
	owner_->script = std::make_shared<Script>();
	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_FALSE(trigger_list.has_triggers_with_rnum(child));
	EXPECT_EQ(trig_index[child]->total_online, 0);
}

TEST_F(LuaTriggerExecTest, ObjectExtractionCancelsWaitingCloneSafely) {
	auto *object = MakeObject();
	const int child = AddPrototype(7451, "mud.wait(10); return true", OBJ_TRIGGER);
	Attach(object->get_script().get(), child);
	const int parent = AddPrototype(7450,
		"return mud.exec(7451, mud.objById(" + std::to_string(object_id_) + "))");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	ASSERT_EQ(Run(parent, context), 1);
	ASSERT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 2u);

	world_objects.AddToExtractedList(object);
	world_objects.PurgeExtractedList();
	object_.reset();
	world_objects.purge();
	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_FALSE(trigger_list.has_triggers_with_rnum(child));
	EXPECT_EQ(trig_index[child]->total_online, 0);
}

TEST_F(LuaTriggerExecTest, RepeatedLaunchesUseIndependentClones) {
	const int child = AddPrototype(7501, R"(
		local run = tonumber(mud.world.get(80, 'runs', 0)) + 1
		mud.world.set(80, 'runs', run)
		mud.wait(run)
		mud.world.set(80, 'done' .. run, 1)
		return true
	)");
	Attach(owner_->script.get(), child);
	const int parent = AddPrototype(7500, R"(
		return mud.exec(7501, ctx.owner) and mud.exec(7501, ctx.owner) and 2 or 0
	)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 2);
	EXPECT_EQ(Eval("mud.world.get(80, 'runs', 0)"), 2);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 3u);
	EXPECT_EQ(trig_index[child]->total_online, 3);

	process_events();
	process_events();
	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_EQ(Eval("mud.world.get(80, 'done1', 0)"), 1);
	EXPECT_EQ(Eval("mud.world.get(80, 'done2', 0)"), 0);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 2u);

	process_events();
	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_EQ(Eval("mud.world.get(80, 'done2', 0)"), 1);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 1u);
	EXPECT_EQ(trig_index[child]->total_online, 1);
}

TEST_F(LuaTriggerExecTest, FailedGlobalRegistrationRollsBackOwnerListAndOnlineCount) {
	const int child = AddPrototype(7551, "return true");
	Attach(owner_->script.get(), child);
	const auto owner_count = CountTriggers(owner_->script->script_trig_list);
	const auto global_count = trigger_list.get_triggers_with_rnum(child).size();
	const auto online = trig_index[child]->total_online;
	lua_scripting::FailNextMudExecGlobalRegistrationForTesting();

	const int parent = AddPrototype(7550, "return mud.exec(7551, ctx.owner) and 0 or 1");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 1);
	EXPECT_EQ(CountTriggers(owner_->script->script_trig_list), owner_count);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), global_count);
	EXPECT_EQ(trig_index[child]->total_online, online);
}

TEST_F(LuaTriggerExecTest, GlobalRebindDefersWaitingCloneCleanupSafely) {
	const int child = AddPrototype(7571, "mud.wait(10); return true");
	Attach(owner_->script.get(), child);
	const int parent = AddPrototype(7570, "return mud.exec(7571, ctx.owner)");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	ASSERT_EQ(Run(parent, context), 1);
	ASSERT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 2u);

	Trigger *clone = nullptr;
	for (auto *trigger : owner_->script->script_trig_list) {
		if (trigger->get_rnum() == child && trigger->is_runned) {
			clone = trigger;
		}
	}
	ASSERT_NE(clone, nullptr);
	trigger_list.remove(clone);
	ASSERT_TRUE(trigger_list.add(clone));
	EXPECT_EQ(CountTriggers(owner_->script->script_trig_list), 2u);

	lua_scripting::LuaScriptEngine::HeartbeatCleanup();
	EXPECT_EQ(CountTriggers(owner_->script->script_trig_list), 1u);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(child).size(), 1u);
	EXPECT_EQ(trig_index[child]->total_online, 1);
}

TEST_F(LuaTriggerExecTest, MutualRecursionStopsAtDepthLimitAndCleansEveryClone) {
	const int a = AddPrototype(7601, "return mud.exec(7602, ctx.owner)");
	const int b = AddPrototype(7602, "return mud.exec(7601, ctx.owner)");
	Attach(owner_->script.get(), a);
	Attach(owner_->script.get(), b);
	const int parent = AddPrototype(7600, "mud.exec(7601, ctx.owner); return 7");
	lua_scripting::LuaTriggerContext context;
	context.trigger = trig_index[parent]->proto;
	context.owner = owner_.get();
	context.trigger_type = MOB_TRIGGER;
	EXPECT_EQ(Run(parent, context), 7);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(a).size(), 1u);
	EXPECT_EQ(trigger_list.get_triggers_with_rnum(b).size(), 1u);
	EXPECT_EQ(trig_index[a]->total_online, 1);
	EXPECT_EQ(trig_index[b]->total_online, 1);
}

} // namespace

#endif // WITH_LUAJIT_PROTOTYPE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
