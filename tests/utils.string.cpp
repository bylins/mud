#include "utils/utils_string.h"

#include <gtest/gtest.h>

#include "utils/utils.h"

struct
{
	const char* data;
	const char* result;
	const char* description;
} test_data[] =
{
	{"", "", "empty string"},
	{"simple string", "simple string", "simple string"},
	{"&a&b&c", "", "just colors"},
	{"&asimple&b &cstring", "simple string", "mixed"},
	{"simple string&", "simple string&", "amp at the tail"},
	{"&&&&&&", "", "amps"}
};

TEST(Utils_String, ProcedureNullTerminated_Null)
{
	EXPECT_NO_FATAL_FAILURE(utils::RemoveColors(nullptr));
}

TEST(Utils_String, ProcedureNullTerminated)
{
	for (const auto& test : test_data)
	{
		utils::shared_string_ptr string(str_dup(test.data), free);
		EXPECT_NO_FATAL_FAILURE(utils::RemoveColors(string.get()));
		EXPECT_EQ(0, strcmp(test.result, string.get()))
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, ProcedureStdString)
{
	for (const auto& test : test_data)
	{
		std::string string = test.data;
		std::string result;
		EXPECT_NO_FATAL_FAILURE(result = utils::RemoveColors(string));
		EXPECT_EQ(test.result, result)
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, FunctionNullTerminated_Null)
{
	EXPECT_EQ(nullptr, utils::GetStringWithoutColors(nullptr));
}

TEST(Utils_String, FunctionNullTerminated)
{
	for (const auto& test : test_data)
	{
		EXPECT_EQ(0, strcmp(test.result, utils::GetStringWithoutColors(test.data).get()))
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, FunctionStdString)
{
	for (const auto& test : test_data)
	{
		std::string string = test.data;
		EXPECT_EQ(test.result, utils::GetStringWithoutColors(string))
			<< "Failed test case '" << test.description << "'";
	}
}

// ===== Split =====

TEST(Utils_String, Split_BasicDelimiter)
{
	auto result = utils::Split("a b c", ' ');
	ASSERT_EQ(3u, result.size());
	EXPECT_EQ("a", result[0]);
	EXPECT_EQ("b", result[1]);
	EXPECT_EQ("c", result[2]);
}

TEST(Utils_String, Split_EmptyString_ReturnsEmpty)
{
	auto result = utils::Split("", ' ');
	EXPECT_TRUE(result.empty());
}

TEST(Utils_String, Split_NoDelimiter_ReturnsSingleElement)
{
	auto result = utils::Split("hello", ' ');
	ASSERT_EQ(1u, result.size());
	EXPECT_EQ("hello", result[0]);
}

TEST(Utils_String, Split_TrailingDelimiter_Ignored)
{
	auto result = utils::Split("a b ", ' ');
	ASSERT_EQ(2u, result.size());
	EXPECT_EQ("a", result[0]);
	EXPECT_EQ("b", result[1]);
}

TEST(Utils_String, Split_CustomDelimiter)
{
	auto result = utils::Split("a,b,c", ',');
	ASSERT_EQ(3u, result.size());
	EXPECT_EQ("a", result[0]);
	EXPECT_EQ("b", result[1]);
	EXPECT_EQ("c", result[2]);
}

// ===== SplitAny =====

TEST(Utils_String, SplitAny_MultipleDelimiters)
{
	auto result = utils::SplitAny("a,b;c", ",;");
	ASSERT_EQ(3u, result.size());
	EXPECT_EQ("a", result[0]);
	EXPECT_EQ("b", result[1]);
	EXPECT_EQ("c", result[2]);
}

TEST(Utils_String, SplitAny_SingleDelimiter_ActsLikeSplit)
{
	auto result = utils::SplitAny("a b c", " ");
	ASSERT_EQ(3u, result.size());
	EXPECT_EQ("a", result[0]);
	EXPECT_EQ("b", result[1]);
	EXPECT_EQ("c", result[2]);
}

// ===== TrimLeft / TrimRight / Trim (std::string) =====

TEST(Utils_String, TrimLeft_StdString_RemovesLeadingSpaces)
{
	std::string s = "   hello";
	utils::TrimLeft(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimLeft_StdString_NoLeadingSpaces_Unchanged)
{
	std::string s = "hello";
	utils::TrimLeft(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimLeft_StdString_Empty_Unchanged)
{
	std::string s;
	utils::TrimLeft(s);
	EXPECT_EQ("", s);
}

TEST(Utils_String, TrimRight_StdString_RemovesTrailingSpaces)
{
	std::string s = "hello   ";
	utils::TrimRight(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimRight_StdString_NoTrailingSpaces_Unchanged)
{
	std::string s = "hello";
	utils::TrimRight(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, Trim_StdString_RemovesBothSides)
{
	std::string s = "  hello  ";
	utils::Trim(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, Trim_StdString_OnlySpaces_BecomesEmpty)
{
	std::string s = "   ";
	utils::Trim(s);
	EXPECT_EQ("", s);
}

// ===== TrimLeftCopy / TrimRightCopy / TrimCopy =====

TEST(Utils_String, TrimLeftCopy_ReturnsNewString)
{
	EXPECT_EQ("hello  ", utils::TrimLeftCopy("  hello  "));
}

TEST(Utils_String, TrimRightCopy_ReturnsNewString)
{
	EXPECT_EQ("  hello", utils::TrimRightCopy("  hello  "));
}

TEST(Utils_String, TrimCopy_ReturnsNewString)
{
	EXPECT_EQ("hello", utils::TrimCopy("  hello  "));
}

// ===== TrimLeft / TrimRight / Trim (char*) =====

TEST(Utils_String, TrimLeft_CharPtr_RemovesLeadingSpaces)
{
	char buf[] = "   hello";
	utils::TrimLeft(buf);
	EXPECT_STREQ("hello", buf);
}

TEST(Utils_String, TrimLeft_CharPtr_NoLeadingSpaces_Unchanged)
{
	char buf[] = "hello";
	utils::TrimLeft(buf);
	EXPECT_STREQ("hello", buf);
}

TEST(Utils_String, TrimRight_CharPtr_RemovesTrailingSpaces)
{
	char buf[] = "hello   ";
	utils::TrimRight(buf);
	EXPECT_STREQ("hello", buf);
}

TEST(Utils_String, TrimRight_CharPtr_NoTrailingSpaces_Unchanged)
{
	char buf[] = "hello";
	utils::TrimRight(buf);
	EXPECT_STREQ("hello", buf);
}

TEST(Utils_String, Trim_CharPtr_RemovesBothSides)
{
	char buf[] = "  hello  ";
	utils::Trim(buf);
	EXPECT_STREQ("hello", buf);
}

// ===== TrimLeftIf / TrimRightIf / TrimIf =====

TEST(Utils_String, TrimLeftIf_RemovesCustomChars)
{
	std::string s = "...hello";
	utils::TrimLeftIf(s, ".");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimRightIf_RemovesCustomChars)
{
	std::string s = "hello...";
	utils::TrimRightIf(s, ".");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimIf_RemovesBothSides)
{
	std::string s = "...hello...";
	utils::TrimIf(s, ".");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, TrimLeftIf_MultipleChars)
{
	std::string s = ".,hello";
	utils::TrimLeftIf(s, ".,");
	EXPECT_EQ("hello", s);
}

// ===== FixDot =====

TEST(Utils_String, FixDot_ReplacesDotsWithSpaces)
{
	EXPECT_EQ("a b", utils::FixDot("a.b"));
}

TEST(Utils_String, FixDot_ReplacesUnderscoresWithSpaces)
{
	EXPECT_EQ("a b", utils::FixDot("a_b"));
}

TEST(Utils_String, FixDot_ReplacesBothDotsAndUnderscores)
{
	EXPECT_EQ("a b c", utils::FixDot("a.b_c"));
}

TEST(Utils_String, FixDot_NoDotsOrUnderscores_Unchanged)
{
	EXPECT_EQ("hello world", utils::FixDot("hello world"));
}

// ===== CompressSymbol =====

TEST(Utils_String, CompressSymbol_RemovesConsecutiveDuplicates)
{
	EXPECT_EQ("a b c", utils::CompressSymbol("a  b  c", ' '));
}

TEST(Utils_String, CompressSymbol_NoDuplicates_Unchanged)
{
	EXPECT_EQ("abc", utils::CompressSymbol("abc", ' '));
}

TEST(Utils_String, CompressSymbol_MultipleRunsCollapsed)
{
	EXPECT_EQ("a b", utils::CompressSymbol("a   b", ' '));
}

// ===== ReplaceFirst =====

TEST(Utils_String, ReplaceFirst_ReplacesFirstOccurrence)
{
	std::string s = "hello hello";
	utils::ReplaceFirst(s, "hello", "world");
	EXPECT_EQ("world hello", s);
}

TEST(Utils_String, ReplaceFirst_NotFound_Unchanged)
{
	std::string s = "hello";
	utils::ReplaceFirst(s, "xyz", "abc");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, ReplaceFirst_SingleOccurrence)
{
	std::string s = "foo bar";
	utils::ReplaceFirst(s, "foo", "baz");
	EXPECT_EQ("baz bar", s);
}

// ===== ReplaceAll =====

TEST(Utils_String, ReplaceAll_ReplacesAllOccurrences)
{
	std::string s = "aaa";
	utils::ReplaceAll(s, "a", "b");
	EXPECT_EQ("bbb", s);
}

TEST(Utils_String, ReplaceAll_NotFound_Unchanged)
{
	std::string s = "hello";
	utils::ReplaceAll(s, "xyz", "abc");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, ReplaceAll_MultipleOccurrences)
{
	std::string s = "foo bar foo baz foo";
	utils::ReplaceAll(s, "foo", "qux");
	EXPECT_EQ("qux bar qux baz qux", s);
}

// ===== EraseAll =====

TEST(Utils_String, EraseAll_RemovesAllOccurrences)
{
	std::string s = "hello world hello";
	utils::EraseAll(s, "hello");
	EXPECT_EQ(" world ", s);
}

TEST(Utils_String, EraseAll_NotFound_Unchanged)
{
	std::string s = "hello";
	utils::EraseAll(s, "xyz");
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, EraseAll_RemovesAllSpaces)
{
	std::string s = "a b c";
	utils::EraseAll(s, " ");
	EXPECT_EQ("abc", s);
}

// ===== ConvertToLow =====

TEST(Utils_String, ConvertToLow_StdString_ConvertsToLowercase)
{
	std::string s = "HELLO";
	utils::ConvertToLow(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, ConvertToLow_StdString_MixedCase)
{
	std::string s = "HeLLo";
	utils::ConvertToLow(s);
	EXPECT_EQ("hello", s);
}

TEST(Utils_String, ConvertToLow_CharPtr_ConvertsToLowercase)
{
	char buf[] = "HELLO";
	utils::ConvertToLow(buf);
	EXPECT_STREQ("hello", buf);
}

// ===== SubstStrToUpper =====

TEST(Utils_String, SubstStrToUpper_ConvertsToUppercase)
{
	EXPECT_EQ("HELLO", utils::SubstStrToUpper("hello"));
}

TEST(Utils_String, SubstStrToUpper_MixedCase)
{
	EXPECT_EQ("HELLO WORLD", utils::SubstStrToUpper("Hello World"));
}

// ===== ExtractFirstArgument =====

TEST(Utils_String, ExtractFirstArgument_ExtractsWord)
{
	std::string remains;
	std::string word = utils::ExtractFirstArgument("hello world", remains);
	EXPECT_EQ("hello", word);
	EXPECT_EQ("world", remains);
}

TEST(Utils_String, ExtractFirstArgument_NoSpace_ReturnsEmpty)
{
	std::string remains;
	std::string word = utils::ExtractFirstArgument("hello", remains);
	EXPECT_TRUE(word.empty());
}

TEST(Utils_String, ExtractFirstArgument_MultipleWords)
{
	std::string remains;
	std::string word = utils::ExtractFirstArgument("one two three", remains);
	EXPECT_EQ("one", word);
	EXPECT_EQ("two three", remains);
}

// ===== FirstWordOnString =====

TEST(Utils_String, FirstWordOnString_ExtractsFirstWord)
{
	EXPECT_EQ("hello", utils::FirstWordOnString("hello world", " "));
}

TEST(Utils_String, FirstWordOnString_NoDelimiter_ReturnsWholeString)
{
	EXPECT_EQ("hello", utils::FirstWordOnString("hello", " "));
}

// ===== CAP =====

TEST(Utils_String, CAP_CharPtr_CapitalizesFirstChar)
{
	char buf[] = "hello";
	utils::CAP(buf);
	EXPECT_STREQ("Hello", buf);
}

TEST(Utils_String, CAP_StdString_CapitalizesFirstChar)
{
	EXPECT_EQ("Hello", utils::CAP(std::string("hello")));
}

TEST(Utils_String, CAP_AlreadyUppercase_Unchanged)
{
	EXPECT_EQ("Hello", utils::CAP(std::string("Hello")));
}

// ===== IsAbbr =====

TEST(Utils_String, IsAbbr_FullMatch_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsAbbr("hello", "hello"));
}

TEST(Utils_String, IsAbbr_PrefixMatch_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsAbbr("hel", "hello"));
}

TEST(Utils_String, IsAbbr_NoMatch_ReturnsFalse)
{
	EXPECT_FALSE(utils::IsAbbr("abc", "hello"));
}

TEST(Utils_String, IsAbbr_EmptyAbbr_ReturnsFalse)
{
	EXPECT_FALSE(utils::IsAbbr("", "hello"));
}

TEST(Utils_String, IsAbbr_CaseInsensitive)
{
	EXPECT_TRUE(utils::IsAbbr("HEL", "hello"));
}

// ===== IsEqual =====

TEST(Utils_String, IsEqual_ExactMatch_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsEqual("hello", "hello"));
}

TEST(Utils_String, IsEqual_NoMatch_ReturnsFalse)
{
	EXPECT_FALSE(utils::IsEqual("world", "hello"));
}

TEST(Utils_String, IsEqual_PrefixAbbrev_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsEqual("hel", "hello"));
}

// ===== IsEquivalent =====

TEST(Utils_String, IsEquivalent_AbbreviatedMatch_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsEquivalent("hel wor", "hello world"));
}

TEST(Utils_String, IsEquivalent_NoMatch_ReturnsFalse)
{
	EXPECT_FALSE(utils::IsEquivalent("xyz", "hello world"));
}

TEST(Utils_String, IsEquivalent_SkipsWords_ReturnsTrue)
{
	EXPECT_TRUE(utils::IsEquivalent("wor", "hello world"));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
