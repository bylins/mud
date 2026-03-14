// Part of Bylins http://www.mud.ru
// Unit tests for MobClassesLoader (Load/Reload with global_vars in XML)

#include "gameplay/classes/mob_classes_info.h"
#include "engine/db/global_objects.h"

#include <gtest/gtest.h>

// Tests that MobClassesLoader::Load and Reload correctly filter out the
// <global_vars> XML node and only process <mobclass> elements.
//
// Regression test for: reload mob_classes returning "Reloading was canceled -
// file damaged" because InfoContainer::Reload() (stop_on_error=true) treated
// the nullptr returned by the builder for <global_vars> as a parse error.

class MobClassesLoaderTest : public ::testing::Test {
protected:
	void SetUp() override {
		mob_classes::MobClassesLoader loader;
		// Initialize with kBoss data. May be a no-op if the global
		// InfoContainer was already initialized by a previous test run,
		// but Reload() below always replaces regardless.
		parser_wrapper::DataNode initial("data/mob_classes/test_initial.xml");
		loader.Load(initial);
	}
};

// After Load with test_initial.xml, global vars and kBoss should be present.
TEST_F(MobClassesLoaderTest, Load_SetsGlobalVarsAndMobClass) {
	EXPECT_EQ(3, mob_classes::GetLvlPerDifficulty());
	EXPECT_EQ(7, mob_classes::GetBossAddLvl());
	EXPECT_TRUE(MUD::MobClasses().IsAvailable(EMobClass::kBoss));
}

// Reload with a different XML (test_updated.xml, kTrash only) should succeed.
// Before the fix, Reload() aborted due to <global_vars> returning nullptr from
// the builder, leaving the old data (kBoss) in place.
// After the fix, Reload() only passes <mobclass> nodes to the builder, so it
// succeeds and the container is replaced with the new data (kTrash, no kBoss).
TEST_F(MobClassesLoaderTest, Reload_WithGlobalVarsInXml_ReplacesData) {
	mob_classes::MobClassesLoader loader;
	parser_wrapper::DataNode updated("data/mob_classes/test_updated.xml");
	loader.Reload(updated);

	EXPECT_EQ(5, mob_classes::GetLvlPerDifficulty());
	EXPECT_EQ(10, mob_classes::GetBossAddLvl());
	EXPECT_TRUE(MUD::MobClasses().IsAvailable(EMobClass::kTrash));
	EXPECT_FALSE(MUD::MobClasses().IsKnown(EMobClass::kBoss));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
