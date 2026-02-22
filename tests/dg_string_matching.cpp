// Part of Bylins http://www.mud.ru
// Unit tests for DG-script string-matching functions:
//   is_substring, word_check, compare_cmd, one_phrase

#include "engine/scripting/dg_triggers.h"

#include <gtest/gtest.h>
#include <cstring>

// ===========================================================================
// is_substring
// ===========================================================================

// "hello" is a whole word found in the middle of the string
TEST(DgStringMatching, IsSubstring_WordInMiddle_ReturnsTrue) {
	EXPECT_EQ(1, is_substring("hello", "say hello world"));
}

// "hello" is a whole word at the start of the string
TEST(DgStringMatching, IsSubstring_WordAtStart_ReturnsTrue) {
	EXPECT_EQ(1, is_substring("hello", "hello world"));
}

// "world" is a whole word at the end of the string
TEST(DgStringMatching, IsSubstring_WordAtEnd_ReturnsTrue) {
	EXPECT_EQ(1, is_substring("world", "hello world"));
}

// "hell" is only a prefix of "hello" -- not a word boundary match
TEST(DgStringMatching, IsSubstring_PartialWord_ReturnsFalse) {
	EXPECT_EQ(0, is_substring("hell", "hello"));
}

// "hello" followed by punctuation still counts as a word
TEST(DgStringMatching, IsSubstring_WordWithPunctuation_ReturnsTrue) {
	EXPECT_EQ(1, is_substring("hello", "hello."));
}

// substring not present at all
TEST(DgStringMatching, IsSubstring_NotFound_ReturnsFalse) {
	EXPECT_EQ(0, is_substring("xyz", "hello world"));
}

// ===========================================================================
// word_check
// ===========================================================================

// wildcard "*" matches any string
TEST(DgStringMatching, WordCheck_Wildcard_ReturnsTrue) {
	EXPECT_EQ(1, word_check("anything at all", "*"));
}

// single word in wordlist matches a word in the string
TEST(DgStringMatching, WordCheck_SingleWordMatch) {
	EXPECT_EQ(1, word_check("hello world", "hello"));
}

// quoted phrase in wordlist must appear as a whole phrase in the string
TEST(DgStringMatching, WordCheck_QuotedPhrase_ReturnsTrue) {
	EXPECT_EQ(1, word_check("hello world", "\"hello world\""));
}

// word not in wordlist returns 0
TEST(DgStringMatching, WordCheck_NoMatch_ReturnsFalse) {
	EXPECT_EQ(0, word_check("hello world", "xyz"));
}

// ===========================================================================
// compare_cmd
// ===========================================================================

// Mode 0: word_check -- source is a wordlist
TEST(DgStringMatching, CompareCmd_Mode0_WordCheck) {
	EXPECT_EQ(1, compare_cmd(0, "say tell", "say"));
	EXPECT_EQ(0, compare_cmd(0, "tell whisper", "say"));
}

// Mode 1: is_substring -- dest must appear as a word in source
TEST(DgStringMatching, CompareCmd_Mode1_Substring) {
	EXPECT_EQ(1, compare_cmd(1, "say hello world", "hello"));
	EXPECT_EQ(0, compare_cmd(1, "say hello world", "bye"));
}

// Mode 2 (default): exact case-sensitive comparison
TEST(DgStringMatching, CompareCmd_Mode2_ExactMatch) {
	EXPECT_EQ(1, compare_cmd(2, "say", "say"));
}

// Mode 2 uses str_cmp which is case-insensitive in this codebase
TEST(DgStringMatching, CompareCmd_Mode2_CaseInsensitive_Match) {
	EXPECT_EQ(1, compare_cmd(2, "say", "Say"));
}

TEST(DgStringMatching, CompareCmd_Mode2_DifferentStrings_NoMatch) {
	EXPECT_EQ(0, compare_cmd(2, "say", "tell"));
}

// Null source returns false
TEST(DgStringMatching, CompareCmd_NullSource_ReturnsFalse) {
	EXPECT_EQ(0, compare_cmd(0, nullptr, "say"));
}

// Wildcard source always returns true (any mode)
TEST(DgStringMatching, CompareCmd_WildcardSource_ReturnsTrue) {
	EXPECT_EQ(1, compare_cmd(0, "*", "anything"));
	EXPECT_EQ(1, compare_cmd(2, "*", "anything"));
}

// ===========================================================================
// one_phrase
// ===========================================================================

// Simple unquoted word: returns the first word, pointer advances to rest
TEST(DgStringMatching, OnePhrase_SimpleWord) {
	char arg[] = "hello world";
	char result[256];
	char *rest = one_phrase(arg, result);
	EXPECT_STREQ("hello", result);
	// rest points into arg at the space or beyond
	EXPECT_STREQ(" world", rest);
}

// Quoted phrase: extracts phrase without quotes, advances past closing quote
TEST(DgStringMatching, OnePhrase_QuotedPhrase) {
	char arg[] = "\"hello world\" rest";
	char result[256];
	char *rest = one_phrase(arg, result);
	EXPECT_STREQ("hello world", result);
	EXPECT_STREQ(" rest", rest);
}

// Empty string: result is empty string, returned pointer is at arg
TEST(DgStringMatching, OnePhrase_EmptyString) {
	char arg[] = "";
	char result[256];
	result[0] = '\xAB';  // sentinel
	one_phrase(arg, result);
	EXPECT_EQ('\0', result[0]);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
