// Part of Bylins http://www.mud.ru
// Unit tests for DG-script string-matching functions:
//   is_substring, word_check, compare_cmd, one_phrase, text_processed (fullword)

#include "engine/scripting/dg_triggers.h"
#include "engine/scripting/dg_scripts.h"

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

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


// ===========================================================================
// text_processed: %var.fullword(слово)%
//
// Регистра в DG нет. add_var_cntx понижает значение переменной у каждого set,
// eval, global и worlds, а подполе приходит из скрипта как написано: у
// %actor.iname% это падеж, то есть имя с большой буквы. Побайтовое сравнение
// их не сводило, и проверка по списку имен была вечно ложной -- молча, без
// следа в логе (в зоне 780 так не работала вся линейка квестов).
// ===========================================================================

namespace {

// 1 -- слово в списке найдено, 0 -- нет, -1 -- поле не обработано
int FullWord(const std::string &list, const std::string &needle) {
	TriggerVar vd;
	vd.name = "spisok";
	vd.value = list;
	vd.context = 0;

	char field[] = "fullword";
	std::vector<char> sub(needle.begin(), needle.end());
	sub.push_back('\0');
	char result[256] = {0};

	if (!text_processed(field, sub.data(), vd, result, sizeof(result))) {
		return -1;
	}
	return std::atoi(result);
}

}  // namespace

// То, на чем сломалась зона 780: список лежит в нижнем регистре, имя приходит с большой
TEST(DgStringMatching, FullWord_CyrillicNameIgnoresCase) {
	EXPECT_EQ(1, FullWord("вукодраг есгар", "Вукодраг"));
	EXPECT_EQ(1, FullWord("Вукодраг Есгар", "вукодраг"));
	EXPECT_EQ(1, FullWord("вукодраг есгар", "ЕСГАР"));
}

TEST(DgStringMatching, FullWord_LatinIgnoresCase) {
	EXPECT_EQ(1, FullWord("alpha beta", "BETA"));
	EXPECT_EQ(1, FullWord("Alpha Beta", "alpha"));
}

// Совпадать должно слово целиком, а не часть
TEST(DgStringMatching, FullWord_PartialWordNotFound) {
	EXPECT_EQ(0, FullWord("вукодраг есгар", "вук"));
	EXPECT_EQ(0, FullWord("вукодраг есгар", "драг"));
	EXPECT_EQ(0, FullWord("alpha beta", "alph"));
}

TEST(DgStringMatching, FullWord_NotInList) {
	EXPECT_EQ(0, FullWord("вукодраг есгар", "кощиха"));
}

// Одно имя в списке и имя с лишними пробелами вокруг списка
TEST(DgStringMatching, FullWord_SingleEntryAndStraySpaces) {
	EXPECT_EQ(1, FullWord("вукодраг", "Вукодраг"));
	EXPECT_EQ(1, FullWord("   вукодраг   есгар  ", "Есгар"));
}

// Пустое подполе не должно находиться ни в каком списке
TEST(DgStringMatching, FullWord_EmptyNeedleNotFound) {
	EXPECT_EQ(0, FullWord("вукодраг есгар", ""));
}

// У пустой переменной поле не обрабатывается -- значение ищется дальше
TEST(DgStringMatching, FullWord_EmptyListIsNotProcessed) {
	EXPECT_EQ(-1, FullWord("", "вукодраг"));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
