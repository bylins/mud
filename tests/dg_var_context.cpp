// Part of Bylins http://www.mud.ru
// Unit tests for DG-script variable-context functions:
//   add_var_cntx, find_var_cntx, remove_var_cntx

#include "engine/scripting/dg_scripts.h"

#include <gtest/gtest.h>
#include <list>
#include <string>

// ===========================================================================
// add_var_cntx / find_var_cntx
// ===========================================================================

// Add a variable and find it back: value matches
TEST(DgVarContext, AddNewVar_FindReturnsValue) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "hello", 0);
	TriggerVar found = find_var_cntx(vars, "myvar", 0);
	EXPECT_EQ("myvar", found.name);
	EXPECT_EQ("hello", found.value);
}

// Re-adding the same name+context updates the value in-place
TEST(DgVarContext, AddSameName_UpdatesValue) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "old", 0);
	add_var_cntx(vars, "myvar", "new", 0);
	TriggerVar found = find_var_cntx(vars, "myvar", 0);
	EXPECT_EQ("new", found.value);
	// List should still have only one entry
	EXPECT_EQ(1u, vars.size());
}

// Searching for a non-existent variable returns a default-constructed TriggerVar
TEST(DgVarContext, FindNotExisting_ReturnsEmpty) {
	std::list<TriggerVar> vars;
	TriggerVar found = find_var_cntx(vars, "ghost", 0);
	EXPECT_TRUE(found.name.empty());
	EXPECT_TRUE(found.value.empty());
}

// ===========================================================================
// remove_var_cntx
// ===========================================================================

// Removing an existing variable returns 1
TEST(DgVarContext, RemoveExisting_ReturnsOne) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "val", 0);
	int result = remove_var_cntx(vars, "myvar", 0);
	EXPECT_EQ(1, result);
}

// Removing a non-existent variable returns 0
TEST(DgVarContext, RemoveNotExisting_ReturnsZero) {
	std::list<TriggerVar> vars;
	int result = remove_var_cntx(vars, "ghost", 0);
	EXPECT_EQ(0, result);
}

// After removal the variable is no longer found
TEST(DgVarContext, FindAfterRemove_ReturnsEmpty) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "val", 0);
	remove_var_cntx(vars, "myvar", 0);
	TriggerVar found = find_var_cntx(vars, "myvar", 0);
	EXPECT_TRUE(found.name.empty());
}

// ===========================================================================
// Context separation
// ===========================================================================

// Same name with different context IDs are independent variables
TEST(DgVarContext, ContextSeparation_SameName_DifferentContexts) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "first",  1);
	add_var_cntx(vars, "myvar", "second", 2);

	EXPECT_EQ("first",  find_var_cntx(vars, "myvar", 1).value);
	EXPECT_EQ("second", find_var_cntx(vars, "myvar", 2).value);
	EXPECT_EQ(2u, vars.size());
}

// A variable stored with context=0 is not found when searching context=1
TEST(DgVarContext, ContextZero_vs_ContextOne_NotVisible) {
	std::list<TriggerVar> vars;
	add_var_cntx(vars, "myvar", "global_val", 0);
	TriggerVar found = find_var_cntx(vars, "myvar", 1);
	EXPECT_TRUE(found.name.empty());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
