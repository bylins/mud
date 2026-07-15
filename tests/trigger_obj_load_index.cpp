// Part of Bylins http://www.mud.ru
// Unit tests for the obj2triggers index (IndexTriggerObjLoads /
// ReindexTriggerObjLoads), which backs the `vnum trig <obj vnum>` command:
// "which triggers load this object".
//
// The index used to be built only by the legacy trigger parser, so on a
// YAML/SQLite world `vnum trig` found nothing. It is now built for every
// trigger of every world format, and refreshed when a script is edited in
// trigedit.

#include "engine/db/world_data_source_base.h"
#include "engine/scripting/dg_scripts.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <string>

namespace {

// Build a trigger whose cmdlist is parsed from the given script, exactly as
// the world loaders do, and index the objects it loads.
void IndexScript(Trigger &trig, TrgVnum trig_vnum, const std::string &script) {
	world_loader::WorldDataSourceBase::ParseTriggerScript(&trig, script);
	IndexTriggerObjLoads(trig_vnum, &trig);
}

bool Indexed(ObjVnum obj_vnum, TrgVnum trig_vnum) {
	const auto it = obj2triggers.find(obj_vnum);
	if (it == obj2triggers.end()) {
		return false;
	}
	return std::find(it->second.begin(), it->second.end(), trig_vnum) != it->second.end();
}

} // namespace

TEST(TriggerObjLoadIndex, IndexesEveryLoadCommandForm) {
	Trigger trig;
	IndexScript(trig, 5001,
				"load obj 1001\n"
				"oload obj 1002\n"
				"mload obj 1003\n"
				"wload obj 1004\n"
				"%load% obj 1005\n");

	EXPECT_TRUE(Indexed(1001, 5001));
	EXPECT_TRUE(Indexed(1002, 5001));
	EXPECT_TRUE(Indexed(1003, 5001));
	EXPECT_TRUE(Indexed(1004, 5001));
	EXPECT_TRUE(Indexed(1005, 5001));
}

// Trigger scripts are indented, and builders type commands in any case.
TEST(TriggerObjLoadIndex, IgnoresIndentationAndCase) {
	Trigger trig;
	IndexScript(trig, 5002,
				"if %actor.level% > 10\n"
				"  MLOAD OBJ 1010\n"
				"end\n");

	EXPECT_TRUE(Indexed(1010, 5002));
}

// do_dgoload()/do_mload() match the argument with IsAbbr(arg, "obj"), so an
// abbreviated "o"/"ob" really does load an object at runtime.
TEST(TriggerObjLoadIndex, IndexesAbbreviatedObjArgument) {
	Trigger trig;
	IndexScript(trig, 5010,
				"mload o 1070\n"
				"oload ob 1071\n");

	EXPECT_TRUE(Indexed(1070, 5010));
	EXPECT_TRUE(Indexed(1071, 5010));
}

// do_dgoload() refuses to load anything unless the vnum is a plain number, so
// neither must the index accept one -- otherwise `vnum trig` reports a trigger
// that in fact loads nothing.
TEST(TriggerObjLoadIndex, SkipsVnumsThatAreNotPlainNumbers) {
	Trigger trig;
	IndexScript(trig, 5011,
				"mload obj %loaditem%\n"
				"mload obj 1080abc\n"
				"mload obj -1081\n"
				"mload obj 99999999999999999999\n");

	EXPECT_EQ(obj2triggers.end(), obj2triggers.find(1080)) << "trailing garbage is not a vnum";
	EXPECT_EQ(obj2triggers.end(), obj2triggers.find(-1081)) << "negative vnum is refused by the runtime";
	EXPECT_FALSE(Indexed(0, 5011)) << "an overflowing vnum must not be indexed as anything";
}

TEST(TriggerObjLoadIndex, SkipsCommandsThatDoNotLoadObjects) {
	Trigger trig;
	IndexScript(trig, 5003,
				"mload mob 1020\n"
				"say load obj 1021\n"
				"mload obj\n"
				"mloadobj 1022\n"
				"mload obj vasya\n");

	EXPECT_FALSE(Indexed(1020, 5003)) << "'mload mob' loads a mob, not an object";
	EXPECT_FALSE(Indexed(1021, 5003)) << "load command must be the first word";
	EXPECT_FALSE(Indexed(1022, 5003)) << "command name must be followed by whitespace";
	EXPECT_EQ(obj2triggers.end(), obj2triggers.find(1022));
}

TEST(TriggerObjLoadIndex, ListsTriggerOnceEvenIfItLoadsObjectTwice) {
	Trigger trig;
	IndexScript(trig, 5004,
				"mload obj 1030\n"
				"mload obj 1030\n");

	const auto it = obj2triggers.find(1030);
	ASSERT_NE(obj2triggers.end(), it);
	EXPECT_EQ(1u, std::count(it->second.begin(), it->second.end(), 5004));
}

TEST(TriggerObjLoadIndex, ListsEveryTriggerLoadingTheSameObject) {
	Trigger first;
	Trigger second;
	IndexScript(first, 5005, "mload obj 1040\n");
	IndexScript(second, 5006, "oload obj 1040\n");

	EXPECT_TRUE(Indexed(1040, 5005));
	EXPECT_TRUE(Indexed(1040, 5006));
}

// After a trigedit save the script may no longer load what it used to.
TEST(TriggerObjLoadIndex, ReindexDropsObjectsTheScriptNoLongerLoads) {
	Trigger trig;
	IndexScript(trig, 5007, "mload obj 1050\n");
	ASSERT_TRUE(Indexed(1050, 5007));

	Trigger edited;
	world_loader::WorldDataSourceBase::ParseTriggerScript(&edited, "mload obj 1051\n");
	ReindexTriggerObjLoads(5007, &edited);

	EXPECT_FALSE(Indexed(1050, 5007)) << "stale entry must be dropped";
	EXPECT_TRUE(Indexed(1051, 5007));
}

// Reindexing one trigger must not evict other triggers loading the same object.
TEST(TriggerObjLoadIndex, ReindexKeepsOtherTriggersOfTheSameObject) {
	Trigger keeper;
	Trigger edited;
	IndexScript(keeper, 5008, "mload obj 1060\n");
	IndexScript(edited, 5009, "mload obj 1060\n");

	Trigger rewritten;
	world_loader::WorldDataSourceBase::ParseTriggerScript(&rewritten, "say nothing to load here\n");
	ReindexTriggerObjLoads(5009, &rewritten);

	EXPECT_TRUE(Indexed(1060, 5008));
	EXPECT_FALSE(Indexed(1060, 5009));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
