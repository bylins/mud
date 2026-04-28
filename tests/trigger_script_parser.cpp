// Part of Bylins http://www.mud.ru
// Unit tests for WorldDataSourceBase::ParseTriggerScript.
//
// Regression tests for line numbering: each parsed cmdlist entry must carry
// a 1-based source line number, matching the legacy parser in
// boot_data_files.cpp:parse_trigger(). Before the fix YAML and SQLite loaders
// left line_num at 0, which broke `tstat -n` output and "trigger ... at line N"
// error reports.

#include "engine/db/world_data_source_base.h"
#include "engine/scripting/dg_scripts.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

// Walk the cmdlist into a flat vector for easier inspection.
std::vector<cmdlist_element::shared_ptr> CollectCommands(const Trigger& trig) {
	std::vector<cmdlist_element::shared_ptr> result;
	if (!trig.cmdlist) {
		return result;
	}
	auto cmd = *trig.cmdlist;
	while (cmd) {
		result.push_back(cmd);
		cmd = cmd->next;
	}
	return result;
}

} // namespace

TEST(ParseTriggerScript, AssignsSequentialLineNumbers) {
	Trigger trig;
	const std::string script =
		"say first\r\n"
		"say second\r\n"
		"say third\r\n";

	world_loader::WorldDataSourceBase::ParseTriggerScript(&trig, script);

	const auto commands = CollectCommands(trig);
	ASSERT_EQ(3u, commands.size());
	EXPECT_EQ(1, commands[0]->line_num);
	EXPECT_EQ(2, commands[1]->line_num);
	EXPECT_EQ(3, commands[2]->line_num);
}

// Empty lines are skipped before line_num is incremented (same as legacy).
TEST(ParseTriggerScript, SkipsEmptyLinesWithoutAdvancingLineNumber) {
	Trigger trig;
	const std::string script =
		"say one\r\n"
		"\r\n"
		"\r\n"
		"say two\r\n";

	world_loader::WorldDataSourceBase::ParseTriggerScript(&trig, script);

	const auto commands = CollectCommands(trig);
	ASSERT_EQ(2u, commands.size());
	EXPECT_EQ(1, commands[0]->line_num);
	EXPECT_EQ(2, commands[1]->line_num);
}

TEST(ParseTriggerScript, EmptyScriptYieldsEmptyList) {
	Trigger trig;
	world_loader::WorldDataSourceBase::ParseTriggerScript(&trig, "");

	ASSERT_TRUE(trig.cmdlist);
	EXPECT_FALSE(*trig.cmdlist) << "Empty script must leave cmdlist empty";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
