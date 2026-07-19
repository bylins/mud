// Part of Bylins http://www.mud.ru
// Unit tests for ExtraDescription

#include "engine/structs/extra_description.h"
#include <vector>

#include <gtest/gtest.h>
#include <memory>
#include <string>

// --- Construction ---

TEST(ExtraDescription, DefaultConstruction_AllFieldsNull) {
	ExtraDescription ed;
	EXPECT_TRUE(ed.keyword.empty());
	EXPECT_TRUE(ed.description.empty());
}

// --- set_keyword ---

TEST(ExtraDescription, SetKeyword_StoresValue) {
	ExtraDescription ed;
	ed.set_keyword("door");
	EXPECT_EQ("door", ed.keyword);
}

TEST(ExtraDescription, SetKeyword_Twice_UpdatesValue) {
	ExtraDescription ed;
	ed.set_keyword("first");
	ed.set_keyword("second");
	EXPECT_EQ("second", ed.keyword);
}

TEST(ExtraDescription, SetKeyword_EmptyString_Stores) {
	ExtraDescription ed;
	ed.set_keyword("");
	EXPECT_EQ("", ed.keyword);
}

// --- set_description ---

TEST(ExtraDescription, SetDescription_StoresValue) {
	ExtraDescription ed;
	ed.set_description("A heavy oak door.");
	EXPECT_EQ("A heavy oak door.", ed.description);
}

TEST(ExtraDescription, SetDescription_Twice_UpdatesValue) {
	ExtraDescription ed;
	ed.set_description("old desc");
	ed.set_description("new desc");
	EXPECT_EQ("new desc", ed.description);
}

// --- Both keyword and description ---

TEST(ExtraDescription, SetBoth_Independent) {
	ExtraDescription ed;
	ed.set_keyword("window");
	ed.set_description("A cracked glass window.");
	EXPECT_EQ("window", ed.keyword);
	EXPECT_EQ("A cracked glass window.", ed.description);
}

// --- Хранение в std::vector (экстра-описания живут в векторе объекта/комнаты) ---

TEST(ExtraDescription, Vector_HoldsMultipleInOrder) {
	std::vector<ExtraDescription> list;
	list.emplace_back();
	list.back().set_keyword("first");
	list.emplace_back();
	list.back().set_keyword("second");

	ASSERT_EQ(2u, list.size());
	EXPECT_EQ("first", list[0].keyword);
	EXPECT_EQ("second", list[1].keyword);
}

// --- Memory management (no leaks - verified by ASAN/valgrind) ---

TEST(ExtraDescription, Destructor_FreesKeyword) {
	// If this test runs under ASAN and doesn't crash, memory is handled correctly
	auto ed = std::make_unique<ExtraDescription>();
	ed->set_keyword("test-key");
	// ed goes out of scope here, destructor should free keyword
}

TEST(ExtraDescription, Destructor_FreesDescription) {
	auto ed = std::make_unique<ExtraDescription>();
	ed->set_description("test-description");
	// ed goes out of scope here, destructor should free description
}

TEST(ExtraDescription, Destructor_WithBoth_FreesBoth) {
	auto ed = std::make_unique<ExtraDescription>();
	ed->set_keyword("key");
	ed->set_description("desc");
	// Both should be freed cleanly
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
