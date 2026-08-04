// Unit tests for the native-encoding character helpers (src/utils/native_text.*, issue #3681).
//
// Pure ASCII: non-ASCII fixtures are spelled as UTF-8 byte escapes. Expectations that differ
// between the KOI8-R and UTF-8 builds branch on native_text::native_is_utf8() (which reflects the
// flag the library was built with), so this one test file is correct under either build.

#include "utils/native_text.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {

// "Privet": 6 Cyrillic code points, 12 UTF-8 bytes. (Name-prefixed: the test files are
// unity-built, so a plain kPrivet would clash with the one in tests/utf8.cpp.)
const char *const kNtPrivet = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

}  // namespace

// The legacy KOI8-R lowercase table (utils.cpp). Declared here rather than including utils.h,
// which pulls in fmt/ and the rest of the engine headers.
extern const char a_lcc_table[];

namespace {

int legacy_lower(unsigned char c) {
	return a_lcc_table[c];
}

}  // namespace

TEST(NativeText, CharCountAscii) {
	EXPECT_EQ(native_text::char_count("Hello"), 5u);
	EXPECT_EQ(native_text::char_count(kNtPrivet, kNtPrivet + 4), native_text::native_is_utf8() ? 2u : 4u);
}

TEST(NativeText, CharCountReflectsEncoding) {
	if (native_text::native_is_utf8()) {
		EXPECT_EQ(native_text::char_count(kNtPrivet), 6u);
		EXPECT_EQ(native_text::char_count(kNtPrivet, kNtPrivet + 12), 6u);
	} else {
		EXPECT_EQ(native_text::char_count(kNtPrivet), 12u);
		EXPECT_EQ(native_text::char_count(kNtPrivet, kNtPrivet + 12), 12u);
	}
}

TEST(NativeText, CapitalizeAscii) {
	char buf[] = "hello";
	native_text::capitalize_first(buf);
	EXPECT_STREQ(buf, "Hello");

	char empty[] = "";
	native_text::capitalize_first(empty);  // must not touch the terminator
	EXPECT_STREQ(empty, "");

	char already[] = "X";
	native_text::capitalize_first(already);
	EXPECT_STREQ(already, "X");
}

TEST(NativeText, CapitalizeCyrillicUtf8Only) {
	if (!native_text::native_is_utf8()) {
		GTEST_SKIP() << "Cyrillic-as-UTF-8 fixtures are only meaningful under the UTF-8 build";
	}
	char buf[] = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";  // "privet"
	native_text::capitalize_first(buf);
	EXPECT_STREQ(buf, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");  // "Privet"
}

TEST(NativeText, CharBytes) {
	EXPECT_EQ(native_text::char_bytes("A"), 1u);
	if (native_text::native_is_utf8()) {
		EXPECT_EQ(native_text::char_bytes(kNtPrivet), 2u);              // Cyrillic lead -> 2 bytes
		EXPECT_EQ(native_text::char_bytes("\xF0\x9F\x98\x80"), 4u);   // 4-byte code point
		EXPECT_EQ(native_text::char_bytes("\xD0"), 1u);              // truncated lead: 1 byte present
	} else {
		EXPECT_EQ(native_text::char_bytes(kNtPrivet), 1u);             // KOI8-R: every byte is a char
	}
}

TEST(NativeText, TruncateOffset) {
	const std::string_view p(kNtPrivet, 12);
	EXPECT_EQ(native_text::truncate_offset(p, 100), 12u);  // past end -> full size
	EXPECT_EQ(native_text::truncate_offset(p, 0), 0u);
	if (native_text::native_is_utf8()) {
		// 5 bytes lands mid-character; back up to the boundary after 2 code points (4 bytes).
		EXPECT_EQ(native_text::truncate_offset(p, 5), 4u);
		EXPECT_EQ(native_text::truncate_offset(p, 4), 4u);
	} else {
		EXPECT_EQ(native_text::truncate_offset(p, 5), 5u);
	}
}

TEST(NativeText, CompareCiAscii) {
	EXPECT_EQ(native_text::compare_ci("abc", "abc"), 0);
	EXPECT_EQ(native_text::compare_ci("ABC", "abc"), 0);   // case-insensitive
	EXPECT_EQ(native_text::compare_ci("AbC", "aBc"), 0);
	EXPECT_LT(native_text::compare_ci("abc", "abd"), 0);   // ordering by first mismatch
	EXPECT_GT(native_text::compare_ci("abd", "abc"), 0);
	EXPECT_LT(native_text::compare_ci("ab", "abc"), 0);    // prefix orders first
	EXPECT_GT(native_text::compare_ci("abc", "ab"), 0);
	EXPECT_EQ(native_text::compare_ci("", ""), 0);
	EXPECT_LT(native_text::compare_ci("", "a"), 0);
}

TEST(NativeText, NCompareCiAscii) {
	EXPECT_EQ(native_text::ncompare_ci("abcdef", "abcXXX", 3), 0);  // only first 3 bytes matter
	EXPECT_NE(native_text::ncompare_ci("abcdef", "abXXXX", 3), 0);
	EXPECT_EQ(native_text::ncompare_ci("ABC", "abc", 3), 0);
	EXPECT_EQ(native_text::ncompare_ci("anything", "other", 0), 0);  // zero budget: equal
	EXPECT_LT(native_text::ncompare_ci("ab", "abc", 10), 0);         // budget beyond the strings
}

TEST(NativeText, CompareCiCyrillicIsCaseInsensitive) {
	// "PRIVET" vs "privet" in Cyrillic: must compare equal in BOTH encodings -- under KOI8-R via
	// the byte table, under UTF-8 via the code-point fold. This is the property that a naive
	// "just compare bytes" UTF-8 migration would silently lose.
	const char *const upper = "\xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2";
	const char *const lower = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
	if (native_text::native_is_utf8()) {
		EXPECT_EQ(native_text::compare_ci(upper, lower), 0);
		EXPECT_EQ(native_text::compare_ci(upper, upper), 0);
		EXPECT_NE(native_text::compare_ci(upper, "\xD0\xBF\xD1\x80\xD0\xB8"), 0);  // prefix differs
	} else {
		// Under KOI8-R these UTF-8 bytes are not Cyrillic; just assert self-equality and that
		// the comparison stays reflexive/antisymmetric on arbitrary high bytes.
		EXPECT_EQ(native_text::compare_ci(upper, upper), 0);
		EXPECT_EQ(native_text::compare_ci(lower, lower), 0);
	}
}

TEST(NativeText, CompareCiMatchesLegacyByteLoopUnderKoi8r) {
	if (native_text::native_is_utf8()) {
		GTEST_SKIP() << "this pins the KOI8-R branch against the original LOWER() byte loop";
	}
	// Reference implementation: the exact loop str_cmp() used before the migration.
	auto legacy = [](const char *a, const char *b) {
		for (int i = 0;; ++i) {
			if (!a[i] && !b[i]) {
				return 0;
			}
			const int chk = legacy_lower(static_cast<unsigned char>(a[i]))
				- legacy_lower(static_cast<unsigned char>(b[i]));
			if (chk != 0) {
				return chk;
			}
		}
	};
	const char *const samples[] = {"", "a", "A", "abc", "ABC", "abd", "ab", "zzz", "\xC1\xC2", "\xE1\xE2"};
	for (const char *x : samples) {
		for (const char *y : samples) {
			EXPECT_EQ(native_text::compare_ci(x, y), legacy(x, y))
				<< "mismatch for \"" << x << "\" vs \"" << y << "\"";
		}
	}
}

TEST(NativeText, IsAlnumChar) {
	EXPECT_TRUE(native_text::is_alnum_char("a"));
	EXPECT_TRUE(native_text::is_alnum_char("Z"));
	EXPECT_TRUE(native_text::is_alnum_char("7"));
	EXPECT_FALSE(native_text::is_alnum_char(" "));
	EXPECT_FALSE(native_text::is_alnum_char("!"));
	EXPECT_FALSE(native_text::is_alnum_char("."));
	EXPECT_FALSE(native_text::is_alnum_char(""));  // terminator is not alphanumeric
	if (native_text::native_is_utf8()) {
		// A Cyrillic letter is ONE alphanumeric character; its trail byte must not be read as
		// punctuation (which is what the raw byte table would do and what breaks tokenisation).
		EXPECT_TRUE(native_text::is_alnum_char(kNtPrivet));
		EXPECT_TRUE(native_text::is_alnum_char("\xD0\x81"));  // Yo
		EXPECT_TRUE(native_text::is_alnum_char("\xD1\x91"));  // yo
	}
}

TEST(NativeText, IsAlphaChar) {
	EXPECT_TRUE(native_text::is_alpha_char("a"));
	EXPECT_TRUE(native_text::is_alpha_char("Z"));
	EXPECT_FALSE(native_text::is_alpha_char("7"));  // digit: alnum but not alpha
	EXPECT_FALSE(native_text::is_alpha_char(" "));
	EXPECT_FALSE(native_text::is_alpha_char(""));
	if (native_text::native_is_utf8()) {
		EXPECT_TRUE(native_text::is_alpha_char(kNtPrivet));
		EXPECT_TRUE(native_text::is_alpha_char("\xD0\x81"));  // Yo
	}
}

TEST(NativeText, IsUpperChar) {
	EXPECT_TRUE(native_text::is_upper_char("A"));
	EXPECT_FALSE(native_text::is_upper_char("a"));
	EXPECT_FALSE(native_text::is_upper_char("7"));
	EXPECT_FALSE(native_text::is_upper_char(""));
	if (native_text::native_is_utf8()) {
		// The byte table cannot see these: a UTF-8 Cyrillic lead byte is outside its uppercase
		// range, which is why the anti-caps filter stopped working for Russian.
		EXPECT_TRUE(native_text::is_upper_char("\xD0\x9F"));   // P
		EXPECT_FALSE(native_text::is_upper_char("\xD0\xBF"));  // p
		EXPECT_TRUE(native_text::is_upper_char("\xD0\x81"));   // Yo
		EXPECT_FALSE(native_text::is_upper_char("\xD1\x91"));  // yo
	}
}

TEST(NativeText, CharsEqualCi) {
	EXPECT_TRUE(native_text::chars_equal_ci("a", "a"));
	EXPECT_TRUE(native_text::chars_equal_ci("a", "A"));
	EXPECT_TRUE(native_text::chars_equal_ci("Z", "z"));
	EXPECT_FALSE(native_text::chars_equal_ci("a", "b"));
	EXPECT_FALSE(native_text::chars_equal_ci("a", ""));
	if (native_text::native_is_utf8()) {
		// The regression this whole step exists for: with the raw KOI8-R byte table the lead
		// bytes of "P"/"p" fold equal but the trail bytes differ, so the match was lost.
		EXPECT_TRUE(native_text::chars_equal_ci("\xD0\x9F", "\xD0\xBF"));   // P vs p
		EXPECT_TRUE(native_text::chars_equal_ci("\xD0\x81", "\xD1\x91"));   // Yo vs yo
		EXPECT_FALSE(native_text::chars_equal_ci("\xD0\x9F", "\xD1\x80"));  // P vs r
	}
}

TEST(NativeText, CopyLowerChar) {
	char buf[8] = {0};
	EXPECT_EQ(native_text::copy_lower_char("A", buf), 1u);
	EXPECT_STREQ(buf, "a");
	EXPECT_EQ(native_text::copy_lower_char("z", buf), 1u);
	EXPECT_STREQ(buf, "z");
	EXPECT_EQ(native_text::copy_lower_char("7", buf), 1u);
	EXPECT_STREQ(buf, "7");
	if (native_text::native_is_utf8()) {
		std::memset(buf, 0, sizeof(buf));
		EXPECT_EQ(native_text::copy_lower_char("\xD0\x9F", buf), 2u);  // P -> p
		EXPECT_STREQ(buf, "\xD0\xBF");
		std::memset(buf, 0, sizeof(buf));
		EXPECT_EQ(native_text::copy_lower_char("\xD0\x81", buf), 2u);  // Yo -> yo
		EXPECT_STREQ(buf, "\xD1\x91");
		// In-place folding must be safe (the lowercase form keeps the byte length).
		char inplace[] = "\xD0\x9F";
		EXPECT_EQ(native_text::copy_lower_char(inplace, inplace), 2u);
		EXPECT_STREQ(inplace, "\xD0\xBF");
	}
}

TEST(NativeText, LastCharOffset) {
	EXPECT_EQ(native_text::last_char_offset(""), 0u);
	EXPECT_EQ(native_text::last_char_offset("a"), 0u);
	EXPECT_EQ(native_text::last_char_offset("abc"), 2u);
	if (native_text::native_is_utf8()) {
		EXPECT_EQ(native_text::last_char_offset(kNtPrivet), 10u);  // 6 chars, last starts at byte 10
		const std::string_view s(kNtPrivet, 12);
		EXPECT_EQ(s.substr(native_text::last_char_offset(s)), "\xD1\x82");  // final char "t"
		EXPECT_EQ(native_text::last_char_offset("\xF0\x9F\x98\x80"), 0u);   // single 4-byte char
	} else {
		EXPECT_EQ(native_text::last_char_offset(kNtPrivet), 11u);  // 12 bytes -> last byte
	}
}

TEST(NativeText, ListContainsChar) {
	EXPECT_TRUE(native_text::list_contains_char("abc", "b"));
	EXPECT_FALSE(native_text::list_contains_char("abc", "d"));
	EXPECT_FALSE(native_text::list_contains_char("abc", ""));
	EXPECT_FALSE(native_text::list_contains_char("", "a"));
	EXPECT_FALSE(native_text::list_contains_char("abc", "A"));  // case-sensitive, like strchr
	if (native_text::native_is_utf8()) {
		// A multibyte character must match as a whole and never on a partial byte sequence.
		const char *const list = "\xD1\x88\xD1\x89\xD0\xB6\xD1\x87";  // sh shch zh ch
		EXPECT_TRUE(native_text::list_contains_char(list, "\xD1\x89"));
		EXPECT_TRUE(native_text::list_contains_char(list, "\xD0\xB6"));
		EXPECT_FALSE(native_text::list_contains_char(list, "\xD1\x82"));
		EXPECT_FALSE(native_text::list_contains_char(list, "\xD1"));  // lead byte alone
	}
}

TEST(NativeText, CharRangeIteratesWholeCharacters) {
	std::vector<std::string> got;
	for (auto c : native_text::chars(kNtPrivet)) {
		got.emplace_back(c);
	}
	if (native_text::native_is_utf8()) {
		ASSERT_EQ(got.size(), 6u);              // 6 letters, not 12 bytes
		EXPECT_EQ(got.front(), "\xD0\x9F");     // whole "P", both bytes
		EXPECT_EQ(got.back(), "\xD1\x82");      // whole "t"
	} else {
		ASSERT_EQ(got.size(), 12u);             // KOI8-R: every byte is a character
	}

	got.clear();
	for (auto c : native_text::chars("abc")) {
		got.emplace_back(c);
	}
	EXPECT_EQ(got, (std::vector<std::string>{"a", "b", "c"}));

	got.clear();
	for (auto c : native_text::chars("")) {
		got.emplace_back(c);
	}
	EXPECT_TRUE(got.empty());
}

TEST(NativeText, WholeStringCaseTransforms) {
	std::string s = "Hello World";
	native_text::to_lower(s);
	EXPECT_EQ(s, "hello world");
	native_text::to_upper(s);
	EXPECT_EQ(s, "HELLO WORLD");

	char buf[] = "MiXeD";
	native_text::to_lower(buf);
	EXPECT_STREQ(buf, "mixed");

	if (native_text::native_is_utf8()) {
		std::string ru = "\xD0\x9F\xD0\xA0\xD0\x98";   // "PRI" in Cyrillic
		native_text::to_lower(ru);
		EXPECT_EQ(ru, "\xD0\xBF\xD1\x80\xD0\xB8");     // "pri"
		native_text::to_upper(ru);
		EXPECT_EQ(ru, "\xD0\x9F\xD0\xA0\xD0\x98");
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
