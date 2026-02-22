// Part of Bylins http://www.mud.ru
// Unit tests for TriggerIndenter

#include "engine/scripting/trigger_indenter.h"

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

// Helper: indent a freshly strdup'd string, assert the result, then free it.
// Usage: EXPECT_INDENT("say hello", 0, "say hello", 0);
static std::string do_indent(TriggerIndenter &indenter, const char *cmd_text, int &level) {
	char *cmd = strdup(cmd_text);
	cmd = indenter.indent(cmd, &level);
	std::string result(cmd);
	free(cmd);
	return result;
}

// --- SimpleCommand_NoIndent ---
// A plain command at level 0 should not be indented and level stays 0.
TEST(TriggerIndenter, SimpleCommand_NoIndent) {
	TriggerIndenter indenter;
	int level = 0;
	EXPECT_EQ("say hello", do_indent(indenter, "say hello", level));
	EXPECT_EQ(0, level);
}

// --- If_OpensBlock ---
// "if" increments level from 0 to 1; the "if" line itself is not indented.
TEST(TriggerIndenter, If_OpensBlock) {
	TriggerIndenter indenter;
	int level = 0;
	EXPECT_EQ("if cond", do_indent(indenter, "if cond", level));
	EXPECT_EQ(1, level);
}

// --- Else_ShiftsLeft ---
// "else" at level 1: current line gets 0 spaces, level stays 1.
TEST(TriggerIndenter, Else_ShiftsLeft) {
	TriggerIndenter indenter;
	// Build up: "if cond" opens a block
	int level = 0;
	do_indent(indenter, "if cond", level);
	ASSERT_EQ(1, level);

	// "else" pulls the current line back to 0 spaces, next level remains 1
	std::string result = do_indent(indenter, "else", level);
	EXPECT_EQ("else", result);
	EXPECT_EQ(1, level);
}

// --- End_ClosesBlock ---
// "end" at level 1: current line gets 0 spaces, level becomes 0.
TEST(TriggerIndenter, End_ClosesBlock) {
	TriggerIndenter indenter;
	int level = 0;
	do_indent(indenter, "if cond", level);
	ASSERT_EQ(1, level);

	std::string result = do_indent(indenter, "end", level);
	EXPECT_EQ("end", result);
	EXPECT_EQ(0, level);
}

// --- While_Done ---
// "while" opens a block (+1), body is indented, "done" closes it (-1).
TEST(TriggerIndenter, While_Done) {
	TriggerIndenter indenter;
	int level = 0;

	// "while" at level 0: not indented, level -> 1
	EXPECT_EQ("while cond", do_indent(indenter, "while cond", level));
	EXPECT_EQ(1, level);

	// body at level 1: 2 spaces
	EXPECT_EQ("  say hi", do_indent(indenter, "say hi", level));
	EXPECT_EQ(1, level);

	// "done" closes: level -> 0
	std::string result = do_indent(indenter, "done", level);
	EXPECT_EQ("done", result);
	EXPECT_EQ(0, level);
}

// --- Switch_Case_Break_End ---
// Verify indentation levels through a full switch/case/break/end sequence.
TEST(TriggerIndenter, Switch_Case_Break_End) {
	TriggerIndenter indenter;
	int level = 0;

	// "switch" at 0: not indented, level -> 1
	EXPECT_EQ("switch var", do_indent(indenter, "switch var", level));
	EXPECT_EQ(1, level);

	// "case 1" at 1: indented 2 spaces, level -> 2
	EXPECT_EQ("  case 1", do_indent(indenter, "case 1", level));
	EXPECT_EQ(2, level);

	// body at 2: indented 4 spaces
	EXPECT_EQ("    say hi", do_indent(indenter, "say hi", level));
	EXPECT_EQ(2, level);

	// "break" pops case, level 2 -> 1
	EXPECT_EQ("  break", do_indent(indenter, "break", level));
	EXPECT_EQ(1, level);

	// "end" closes switch, level 1 -> 0
	EXPECT_EQ("end", do_indent(indenter, "end", level));
	EXPECT_EQ(0, level);
}

// --- Nested_If_While ---
// if/while nesting: verify correct indentation at each depth.
TEST(TriggerIndenter, Nested_If_While) {
	TriggerIndenter indenter;
	int level = 0;

	// "if" at 0: no indent, level -> 1
	EXPECT_EQ("if cond", do_indent(indenter, "if cond", level));
	EXPECT_EQ(1, level);

	// "while" at 1: 2 spaces indent, level -> 2
	EXPECT_EQ("  while cond", do_indent(indenter, "while cond", level));
	EXPECT_EQ(2, level);

	// body at 2: 4 spaces
	EXPECT_EQ("    say hi", do_indent(indenter, "say hi", level));
	EXPECT_EQ(2, level);

	// "done" closes while: level 2 -> 1
	EXPECT_EQ("  done", do_indent(indenter, "done", level));
	EXPECT_EQ(1, level);

	// body between while and end at 1: 2 spaces
	EXPECT_EQ("  say bye", do_indent(indenter, "say bye", level));
	EXPECT_EQ(1, level);

	// "end" closes if: level 1 -> 0
	EXPECT_EQ("end", do_indent(indenter, "end", level));
	EXPECT_EQ(0, level);
}

// --- LevelClampAtZero ---
// Calling "end" at level 0 should not produce a negative level.
TEST(TriggerIndenter, LevelClampAtZero) {
	TriggerIndenter indenter;
	int level = 0;
	do_indent(indenter, "end", level);
	EXPECT_GE(level, 0);
}

// --- Reset_ClearsStack ---
// After reset(), indenter starts fresh regardless of prior state.
TEST(TriggerIndenter, Reset_ClearsStack) {
	TriggerIndenter indenter;
	int level = 0;

	// Build up state
	do_indent(indenter, "if cond", level);
	ASSERT_EQ(1, level);

	// Explicitly reset
	indenter.reset();

	// After reset, start at level 0 again -- plain command should not be indented
	level = 0;
	EXPECT_EQ("say hi", do_indent(indenter, "say hi", level));
	EXPECT_EQ(0, level);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
