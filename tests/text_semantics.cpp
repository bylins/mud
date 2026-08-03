// Regression tests for the byte-vs-character migration (issue #3681).
//
// These pin the *observable behaviour* of the text routines that used to assume 1 byte == 1
// character: name matching, case-insensitive comparison, argument splitting and Russian name
// declension. Several of these paths had no coverage at all before the migration touched them.
//
// The Russian literals below are deliberately written as literals rather than byte escapes: the
// file is compiled in whatever encoding the engine is built with (KOI8-R today, UTF-8 after the
// flip), and the routines under test operate in that same native encoding. The expectations are
// therefore valid in both, and this file doubles as the guard that the flip did not change
// user-visible behaviour.

#include "utils/utils_string.h"
#include "utils/mud_string.h"
#include "gameplay/core/genchar.h"
#include "utils/grammar/gender.h"
#include "engine/structs/structs.h"

#include <gtest/gtest.h>

#include <string>

namespace {

std::string declension(const char *name, EGender sex, int case_num) {
	char buf[128] = {0};
	GetCase(name, sex, case_num, buf);
	return std::string(buf);
}

std::string first_argument(const char *line) {
	char buf[kMaxInputLength] = {0};
	one_argument(line, buf);
	return std::string(buf);
}

}  // namespace

// ---------------------------------------------------------------------------- isname

TEST(TextSemantics, IsnameMatchesAsciiKeywords) {
	EXPECT_TRUE(isname("sword", "a long sword"));
	EXPECT_TRUE(isname("long", "a long sword"));
	EXPECT_TRUE(isname("SWORD", "a long sword"));  // case-insensitive
	EXPECT_TRUE(isname("swo", "a long sword"));    // prefix
	EXPECT_FALSE(isname("xyzzy", "a long sword"));
}

TEST(TextSemantics, IsnameMatchesRussianKeywords) {
	EXPECT_TRUE(isname("меч", "меч длинный"));
	EXPECT_TRUE(isname("длинный", "меч длинный"));
	EXPECT_TRUE(isname("ме", "меч длинный"));  // prefix
}

TEST(TextSemantics, IsnameIsCaseInsensitiveForRussian) {
	// Regression: with byte-wise folding under UTF-8 the lead bytes of an upper/lower Cyrillic
	// letter fold equal but the trail bytes do not, so this match was silently lost.
	EXPECT_TRUE(isname("МЕЧ", "меч"));
	EXPECT_TRUE(isname("Меч", "меч длинный"));
	EXPECT_TRUE(isname("меч", "МЕЧ ДЛИННЫЙ"));
}

TEST(TextSemantics, IsnameRejectsUnrelatedRussianWords) {
	// Regression: byte-wise matching under UTF-8 compared only the shared leading byte of two
	// different Cyrillic letters, so unrelated words matched each other.
	EXPECT_FALSE(isname("щит", "меч длинный"));
	EXPECT_FALSE(isname("меч", "щит деревянный"));
	EXPECT_FALSE(isname("кольцо", "меч"));
}

// ---------------------------------------------------------------------------- str_cmp

TEST(TextSemantics, StrCmpIgnoresCase) {
	EXPECT_EQ(str_cmp("abc", "ABC"), 0);
	EXPECT_EQ(str_cmp("меч", "МЕЧ"), 0);
	EXPECT_EQ(str_cmp(std::string("меч"), "МЕЧ"), 0);
	EXPECT_NE(str_cmp("меч", "щит"), 0);
}

TEST(TextSemantics, StrCmpOrdersConsistently) {
	EXPECT_LT(str_cmp("abc", "abd"), 0);
	EXPECT_GT(str_cmp("abd", "abc"), 0);
	EXPECT_LT(str_cmp("ab", "abc"), 0);  // prefix sorts first
	EXPECT_GT(str_cmp("abc", "ab"), 0);
}

TEST(TextSemantics, StrnCmpComparesPrefixOnly) {
	EXPECT_EQ(strn_cmp("abcdef", "abcXXX", 3), 0);
	EXPECT_NE(strn_cmp("abcdef", "abXXXX", 3), 0);
	EXPECT_EQ(strn_cmp("МЕЧ", "меч", 6), 0);
}

// ---------------------------------------------------------------------------- argument splitting

TEST(TextSemantics, OneArgumentLowercasesAscii) {
	EXPECT_EQ(first_argument("LOOK north"), "look");
	EXPECT_EQ(first_argument("   Kill  orc"), "kill");
}

TEST(TextSemantics, OneArgumentLowercasesRussian) {
	// Regression: the byte table leaves UTF-8 Cyrillic untouched, so a Russian command argument
	// would reach the command lookup unfolded and fail to match.
	EXPECT_EQ(first_argument("СМОТРЕТЬ север"), "смотреть");
	EXPECT_EQ(first_argument("Убить орка"), "убить");
	EXPECT_EQ(first_argument("меч"), "меч");
}

TEST(TextSemantics, HalfChopSplitsAndLowercasesFirstWord) {
	char arg1[kMaxInputLength] = {0};
	char arg2[kMaxInputLength] = {0};
	half_chop("СКАЗАТЬ привет всем", arg1, arg2);
	EXPECT_STREQ(arg1, "сказать");
	EXPECT_STREQ(arg2, "привет всем");
}

// ---------------------------------------------------------------------------- GetCase

TEST(TextSemantics, DeclensionOfFeminineNameEndingInYa) {
	// Regression: under UTF-8 the byte-wise last-letter test never matched, so names stopped
	// declining entirely and every case returned the nominative.
	EXPECT_EQ(declension("Аня", EGender::kFemale, 1), "Ани");
	EXPECT_EQ(declension("Аня", EGender::kFemale, 2), "Ане");
	EXPECT_EQ(declension("Аня", EGender::kFemale, 3), "Аню");
	EXPECT_EQ(declension("Аня", EGender::kFemale, 4), "Аней");
	EXPECT_EQ(declension("Аня", EGender::kFemale, 5), "Ане");
}

TEST(TextSemantics, DeclensionOfMasculineNameEndingInConsonant) {
	EXPECT_EQ(declension("Иван", EGender::kMale, 1), "Ивана");
	EXPECT_EQ(declension("Иван", EGender::kMale, 2), "Ивану");
	EXPECT_EQ(declension("Иван", EGender::kMale, 4), "Иваном");
	EXPECT_EQ(declension("Иван", EGender::kMale, 5), "Иване");
}

TEST(TextSemantics, DeclensionOfNameEndingInA) {
	// The genitive/instrumental endings depend on the letter *before* the final one.
	EXPECT_EQ(declension("Маша", EGender::kFemale, 1), "Маши");   // after ш -> и
	EXPECT_EQ(declension("Анна", EGender::kFemale, 1), "Анны");   // otherwise -> ы
	EXPECT_EQ(declension("Маша", EGender::kFemale, 4), "Машей");  // after ш -> ей
	EXPECT_EQ(declension("Анна", EGender::kFemale, 4), "Анной");  // otherwise -> ой
}

TEST(TextSemantics, DeclensionOfMasculineNameEndingInIShort) {
	EXPECT_EQ(declension("Дрегвий", EGender::kMale, 1), "Дрегвия");
	EXPECT_EQ(declension("Дрегвий", EGender::kMale, 4), "Дрегвием");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
