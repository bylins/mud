// Part of Bylins http://www.mud.ru
// Unit tests for ExtraDescription

#include "engine/structs/extra_description.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

// --- Construction ---

TEST(ExtraDescription, DefaultConstruction_AllFieldsNull) {
	ExtraDescription ed;
	EXPECT_EQ(nullptr, ed.keyword);
	EXPECT_EQ(nullptr, ed.description);
	EXPECT_EQ(nullptr, ed.next);
}

// --- set_keyword ---

TEST(ExtraDescription, SetKeyword_StoresValue) {
	ExtraDescription ed;
	ed.set_keyword("door");
	ASSERT_NE(nullptr, ed.keyword);
	EXPECT_STREQ("door", ed.keyword);
}

TEST(ExtraDescription, SetKeyword_Twice_UpdatesValue) {
	ExtraDescription ed;
	ed.set_keyword("first");
	ed.set_keyword("second");
	EXPECT_STREQ("second", ed.keyword);
}

TEST(ExtraDescription, SetKeyword_EmptyString_Stores) {
	ExtraDescription ed;
	ed.set_keyword("");
	ASSERT_NE(nullptr, ed.keyword);
	EXPECT_STREQ("", ed.keyword);
}

// --- set_description ---

TEST(ExtraDescription, SetDescription_StoresValue) {
	ExtraDescription ed;
	ed.set_description("A heavy oak door.");
	ASSERT_NE(nullptr, ed.description);
	EXPECT_STREQ("A heavy oak door.", ed.description);
}

TEST(ExtraDescription, SetDescription_Twice_UpdatesValue) {
	ExtraDescription ed;
	ed.set_description("old desc");
	ed.set_description("new desc");
	EXPECT_STREQ("new desc", ed.description);
}

// --- Both keyword and description ---

TEST(ExtraDescription, SetBoth_Independent) {
	ExtraDescription ed;
	ed.set_keyword("window");
	ed.set_description("A cracked glass window.");
	EXPECT_STREQ("window", ed.keyword);
	EXPECT_STREQ("A cracked glass window.", ed.description);
}

// --- Linked list via shared_ptr ---

TEST(ExtraDescription, LinkedList_NextIsNull_ByDefault) {
	auto ed = std::make_shared<ExtraDescription>();
	EXPECT_EQ(nullptr, ed->next);
}

TEST(ExtraDescription, LinkedList_CanChain) {
	auto head = std::make_shared<ExtraDescription>();
	auto second = std::make_shared<ExtraDescription>();
	auto third = std::make_shared<ExtraDescription>();

	head->set_keyword("head");
	second->set_keyword("second");
	third->set_keyword("third");

	head->next = second;
	second->next = third;

	// Traverse
	EXPECT_STREQ("head", head->keyword);
	EXPECT_STREQ("second", head->next->keyword);
	EXPECT_STREQ("third", head->next->next->keyword);
	EXPECT_EQ(nullptr, head->next->next->next);
}

TEST(ExtraDescription, LinkedList_DropHead_SecondBecomesReachable) {
	auto head = std::make_shared<ExtraDescription>();
	auto second = std::make_shared<ExtraDescription>();
	head->set_keyword("head");
	second->set_keyword("second");
	head->next = second;

	// Drop head - second should still be alive
	head.reset();
	EXPECT_STREQ("second", second->keyword);
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
