// Unit tests for the native-encoding character helpers (src/utils/native_text.*, issue #3681).
//
// Pure ASCII: non-ASCII fixtures are spelled as UTF-8 byte escapes, so the test data does not
// depend on how an editor happens to save this file.

#include "utils/native_text.h"
#include "utils/utf8.h"
#include "utils/russian_keys.h"

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


namespace {


}  // namespace

TEST(NativeText, CharCountAscii) {
	EXPECT_EQ(native_text::char_count("Hello"), 5u);
	EXPECT_EQ(native_text::char_count(kNtPrivet, kNtPrivet + 4), 2u);
}

TEST(NativeText, CharCountReflectsEncoding) {
	EXPECT_EQ(native_text::char_count(kNtPrivet), 6u);
	EXPECT_EQ(native_text::char_count(kNtPrivet, kNtPrivet + 12), 6u);
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

TEST(NativeText, CapitalizeCyrillic) {
	char buf[] = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";  // "privet"
	native_text::capitalize_first(buf);
	EXPECT_STREQ(buf, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");  // "Privet"
}

TEST(NativeText, CharBytes) {
	EXPECT_EQ(native_text::char_bytes("A"), 1u);
	EXPECT_EQ(native_text::char_bytes(kNtPrivet), 2u);              // Cyrillic lead -> 2 bytes
	EXPECT_EQ(native_text::char_bytes("\xF0\x9F\x98\x80"), 4u);   // 4-byte code point
	EXPECT_EQ(native_text::char_bytes("\xD0"), 1u);              // truncated lead: 1 byte present
}

TEST(NativeText, TruncateOffset) {
	const std::string_view p(kNtPrivet, 12);
	EXPECT_EQ(native_text::truncate_offset(p, 100), 12u);  // past end -> full size
	EXPECT_EQ(native_text::truncate_offset(p, 0), 0u);
	// 5 bytes lands mid-character; back up to the boundary after 2 code points (4 bytes).
	EXPECT_EQ(native_text::truncate_offset(p, 5), 4u);
	EXPECT_EQ(native_text::truncate_offset(p, 4), 4u);
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

TEST(NativeText, CompareCiCyrillicIsCaseInsensitive) {
	// "PRIVET" vs "privet" in Cyrillic: must compare equal in BOTH encodings -- under KOI8-R via
	// the byte table, under UTF-8 via the code-point fold. This is the property that a naive
	// "just compare bytes" UTF-8 migration would silently lose.
	const char *const upper = "\xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2";
	const char *const lower = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
	EXPECT_EQ(native_text::compare_ci(upper, lower), 0);
	EXPECT_EQ(native_text::compare_ci(upper, upper), 0);
	EXPECT_NE(native_text::compare_ci(upper, "\xD0\xBF\xD1\x80\xD0\xB8"), 0);  // prefix differs
}

TEST(NativeText, IsAlnumChar) {
	EXPECT_TRUE(native_text::is_alnum_char("a"));
	EXPECT_TRUE(native_text::is_alnum_char("Z"));
	EXPECT_TRUE(native_text::is_alnum_char("7"));
	EXPECT_FALSE(native_text::is_alnum_char(" "));
	EXPECT_FALSE(native_text::is_alnum_char("!"));
	EXPECT_FALSE(native_text::is_alnum_char("."));
	EXPECT_FALSE(native_text::is_alnum_char(""));  // terminator is not alphanumeric
	// A Cyrillic letter is ONE alphanumeric character; its trail byte must not be read as
	// punctuation (which is what the raw byte table would do and what breaks tokenisation).
	EXPECT_TRUE(native_text::is_alnum_char(kNtPrivet));
	EXPECT_TRUE(native_text::is_alnum_char("\xD0\x81"));  // Yo
	EXPECT_TRUE(native_text::is_alnum_char("\xD1\x91"));  // yo
}

TEST(NativeText, IsAlphaChar) {
	EXPECT_TRUE(native_text::is_alpha_char("a"));
	EXPECT_TRUE(native_text::is_alpha_char("Z"));
	EXPECT_FALSE(native_text::is_alpha_char("7"));  // digit: alnum but not alpha
	EXPECT_FALSE(native_text::is_alpha_char(" "));
	EXPECT_FALSE(native_text::is_alpha_char(""));
	EXPECT_TRUE(native_text::is_alpha_char(kNtPrivet));
	EXPECT_TRUE(native_text::is_alpha_char("\xD0\x81"));  // Yo
}

TEST(NativeText, IsUpperChar) {
	EXPECT_TRUE(native_text::is_upper_char("A"));
	EXPECT_FALSE(native_text::is_upper_char("a"));
	EXPECT_FALSE(native_text::is_upper_char("7"));
	EXPECT_FALSE(native_text::is_upper_char(""));
	// The byte table cannot see these: a UTF-8 Cyrillic lead byte is outside its uppercase
	// range, which is why the anti-caps filter stopped working for Russian.
	EXPECT_TRUE(native_text::is_upper_char("\xD0\x9F"));   // P
	EXPECT_FALSE(native_text::is_upper_char("\xD0\xBF"));  // p
	EXPECT_TRUE(native_text::is_upper_char("\xD0\x81"));   // Yo
	EXPECT_FALSE(native_text::is_upper_char("\xD1\x91"));  // yo
}

TEST(NativeText, CharsEqualCi) {
	EXPECT_TRUE(native_text::chars_equal_ci("a", "a"));
	EXPECT_TRUE(native_text::chars_equal_ci("a", "A"));
	EXPECT_TRUE(native_text::chars_equal_ci("Z", "z"));
	EXPECT_FALSE(native_text::chars_equal_ci("a", "b"));
	EXPECT_FALSE(native_text::chars_equal_ci("a", ""));
	// The regression this whole step exists for: with the raw KOI8-R byte table the lead
	// bytes of "P"/"p" fold equal but the trail bytes differ, so the match was lost.
	EXPECT_TRUE(native_text::chars_equal_ci("\xD0\x9F", "\xD0\xBF"));   // P vs p
	EXPECT_TRUE(native_text::chars_equal_ci("\xD0\x81", "\xD1\x91"));   // Yo vs yo
	EXPECT_FALSE(native_text::chars_equal_ci("\xD0\x9F", "\xD1\x80"));  // P vs r
}

TEST(NativeText, CopyLowerChar) {
	char buf[8] = {0};
	EXPECT_EQ(native_text::copy_lower_char("A", buf), 1u);
	EXPECT_STREQ(buf, "a");
	EXPECT_EQ(native_text::copy_lower_char("z", buf), 1u);
	EXPECT_STREQ(buf, "z");
	EXPECT_EQ(native_text::copy_lower_char("7", buf), 1u);
	EXPECT_STREQ(buf, "7");
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

TEST(NativeText, LastCharOffset) {
	EXPECT_EQ(native_text::last_char_offset(""), 0u);
	EXPECT_EQ(native_text::last_char_offset("a"), 0u);
	EXPECT_EQ(native_text::last_char_offset("abc"), 2u);
	EXPECT_EQ(native_text::last_char_offset(kNtPrivet), 10u);  // 6 chars, last starts at byte 10
	const std::string_view s(kNtPrivet, 12);
	EXPECT_EQ(s.substr(native_text::last_char_offset(s)), "\xD1\x82");  // final char "t"
	EXPECT_EQ(native_text::last_char_offset("\xF0\x9F\x98\x80"), 0u);   // single 4-byte char
}

TEST(NativeText, ListContainsChar) {
	EXPECT_TRUE(native_text::list_contains_char("abc", "b"));
	EXPECT_FALSE(native_text::list_contains_char("abc", "d"));
	EXPECT_FALSE(native_text::list_contains_char("abc", ""));
	EXPECT_FALSE(native_text::list_contains_char("", "a"));
	EXPECT_FALSE(native_text::list_contains_char("abc", "A"));  // case-sensitive, like strchr
	// A multibyte character must match as a whole and never on a partial byte sequence.
	const char *const list = "\xD1\x88\xD1\x89\xD0\xB6\xD1\x87";  // sh shch zh ch
	EXPECT_TRUE(native_text::list_contains_char(list, "\xD1\x89"));
	EXPECT_TRUE(native_text::list_contains_char(list, "\xD0\xB6"));
	EXPECT_FALSE(native_text::list_contains_char(list, "\xD1\x82"));
	EXPECT_FALSE(native_text::list_contains_char(list, "\xD1"));  // lead byte alone
}

TEST(NativeText, CharRangeIteratesWholeCharacters) {
	std::vector<std::string> got;
	for (auto c : native_text::chars(kNtPrivet)) {
		got.emplace_back(c);
	}
	ASSERT_EQ(got.size(), 6u);              // 6 letters, not 12 bytes
	EXPECT_EQ(got.front(), "\xD0\x9F");     // whole "P", both bytes
	EXPECT_EQ(got.back(), "\xD1\x82");      // whole "t"

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

	std::string ru = "\xD0\x9F\xD0\xA0\xD0\x98";   // "PRI" in Cyrillic
	native_text::to_lower(ru);
	EXPECT_EQ(ru, "\xD0\xBF\xD1\x80\xD0\xB8");     // "pri"
	native_text::to_upper(ru);
	EXPECT_EQ(ru, "\xD0\x9F\xD0\xA0\xD0\x98");
}

TEST(NativeText, RussianKeysMatchFirstCharCode) {
	// The switch-dispatch contract: for every Russian letter, the constant in russian_keys.h must
	// equal what first_char_code() returns for that letter in the build's native encoding. If this
	// ever drifts, menus and OLC editors silently stop responding to that key.
	struct Case { const char *koi8; const char *utf8; char32_t expected; };
	const Case cases[] = {
		{"\xC1", "\xD0\xB0", rus::kA},    {"\xE1", "\xD0\x90", rus::kAUpper},
		{"\xC4", "\xD0\xB4", rus::kDe},   {"\xE4", "\xD0\x94", rus::kDeUpper},
		{"\xCE", "\xD0\xBD", rus::kEn},   {"\xEE", "\xD0\x9D", rus::kEnUpper},
		{"\xD1", "\xD1\x8F", rus::kYa},   {"\xF1", "\xD0\xAF", rus::kYaUpper},
		{"\xA3", "\xD1\x91", rus::kYo},   {"\xB3", "\xD0\x81", rus::kYoUpper},
		{"\xD7", "\xD0\xB2", rus::kVe},   {"\xC8", "\xD1\x85", rus::kHa},
	};
	for (const auto &c : cases) {
		const char *const input = c.utf8;
		EXPECT_EQ(native_text::first_char_code(input), c.expected);
	}

	// ASCII keys are unchanged by the flip and stay ordinary character literals in the switches.
	EXPECT_EQ(native_text::first_char_code("y"), static_cast<char32_t>('y'));
	EXPECT_EQ(native_text::first_char_code("N"), static_cast<char32_t>('N'));
	EXPECT_EQ(native_text::first_char_code(""), 0u);
	EXPECT_EQ(native_text::first_char_code(nullptr), 0u);
}

TEST(NativeText, TransliterationIsStableAcrossTheFlip) {
	// A player's save file is named after the transliterated character name, so this mapping is
	// on-disk state: if it ever changes, every existing character stops being found. Each row is
	// the letter in both encodings and the single ASCII character it must always produce -- the
	// values were taken from what the byte-wise implementation produced before the migration.
	struct Row { const char *koi8; const char *utf8; char expected; };
	static const Row kRows[] = {
		{"\xC1", "\xD0\xB0", 'a'},
		{"\xE1", "\xD0\x90", 'a'},
		{"\xC2", "\xD0\xB1", 'b'},
		{"\xE2", "\xD0\x91", 'b'},
		{"\xD7", "\xD0\xB2", 'v'},
		{"\xF7", "\xD0\x92", 'v'},
		{"\xC7", "\xD0\xB3", 'g'},
		{"\xE7", "\xD0\x93", 'g'},
		{"\xC4", "\xD0\xB4", 'd'},
		{"\xE4", "\xD0\x94", 'd'},
		{"\xC5", "\xD0\xB5", 'e'},
		{"\xE5", "\xD0\x95", 'e'},
		{"\xA3", "\xD1\x91", '9'},
		{"\xB3", "\xD0\x81", '9'},
		{"\xD6", "\xD0\xB6", '1'},
		{"\xF6", "\xD0\x96", '1'},
		{"\xDA", "\xD0\xB7", 'z'},
		{"\xFA", "\xD0\x97", 'z'},
		{"\xC9", "\xD0\xB8", 'i'},
		{"\xE9", "\xD0\x98", 'i'},
		{"\xCA", "\xD0\xB9", 'j'},
		{"\xEA", "\xD0\x99", 'j'},
		{"\xCB", "\xD0\xBA", 'k'},
		{"\xEB", "\xD0\x9A", 'k'},
		{"\xCC", "\xD0\xBB", 'l'},
		{"\xEC", "\xD0\x9B", 'l'},
		{"\xCD", "\xD0\xBC", 'm'},
		{"\xED", "\xD0\x9C", 'm'},
		{"\xCE", "\xD0\xBD", 'n'},
		{"\xEE", "\xD0\x9D", 'n'},
		{"\xCF", "\xD0\xBE", 'o'},
		{"\xEF", "\xD0\x9E", 'o'},
		{"\xD0", "\xD0\xBF", 'p'},
		{"\xF0", "\xD0\x9F", 'p'},
		{"\xD2", "\xD1\x80", 'r'},
		{"\xF2", "\xD0\xA0", 'r'},
		{"\xD3", "\xD1\x81", 's'},
		{"\xF3", "\xD0\xA1", 's'},
		{"\xD4", "\xD1\x82", 't'},
		{"\xF4", "\xD0\xA2", 't'},
		{"\xD5", "\xD1\x83", 'y'},
		{"\xF5", "\xD0\xA3", 'y'},
		{"\xC6", "\xD1\x84", 'f'},
		{"\xE6", "\xD0\xA4", 'f'},
		{"\xC8", "\xD1\x85", 'h'},
		{"\xE8", "\xD0\xA5", 'h'},
		{"\xC3", "\xD1\x86", 'c'},
		{"\xE3", "\xD0\xA6", 'c'},
		{"\xDE", "\xD1\x87", '7'},
		{"\xFE", "\xD0\xA7", '7'},
		{"\xDB", "\xD1\x88", '4'},
		{"\xFB", "\xD0\xA8", '4'},
		{"\xDD", "\xD1\x89", '6'},
		{"\xFD", "\xD0\xA9", '6'},
		{"\xDF", "\xD1\x8A", '8'},
		{"\xFF", "\xD0\xAA", '8'},
		{"\xD9", "\xD1\x8B", '3'},
		{"\xF9", "\xD0\xAB", '3'},
		{"\xD8", "\xD1\x8C", '2'},
		{"\xF8", "\xD0\xAC", '2'},
		{"\xDC", "\xD1\x8D", '5'},
		{"\xFC", "\xD0\xAD", '5'},
		{"\xC0", "\xD1\x8E", '0'},
		{"\xE0", "\xD0\xAE", '0'},
		{"\xD1", "\xD1\x8F", 'q'},
		{"\xF1", "\xD0\xAF", 'q'},
	};
	for (const auto &r : kRows) {
		const char *const input = r.utf8;
		EXPECT_EQ(native_text::translit_to_filename(input), std::string(1, r.expected))
			<< "transliteration drifted for " << r.utf8;
	}

	// ASCII is lowercased and digits pass through, as before.
	EXPECT_EQ(native_text::translit_to_filename("Vasya"), "vasya");
	EXPECT_EQ(native_text::translit_to_filename("Abc123"), "abc123");
	EXPECT_EQ(native_text::translit_to_filename(""), "");

	// A whole name: "Vasya" in Cyrillic must give the same file name in both encodings.
	const char *const name = "\xD0\x92\xD0\xB0\xD1\x81\xD1\x8F";   // "Vasya"
	EXPECT_EQ(native_text::translit_to_filename(name), "vasq");
}

TEST(NativeText, FromKoi8BringsDiskTextIntoTheNativeEncoding) {
	// Data files (world, configs, help, boards, saves) are stored in KOI8-R. from_koi8() is the
	// boundary that brings them into whatever the engine runs on: a no-op today, a transcode
	// after the flip. Both directions are asserted here so the wiring can be trusted before the
	// sources that need it are converted.
	const char *const koi8_privet = "\xF0\xD2\xC9\xD7\xC5\xD4";              // "Privet", KOI8-R
	const char *const utf8_privet = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";  // the same, UTF-8

	EXPECT_EQ(native_text::from_koi8(""), "");
	EXPECT_EQ(native_text::from_koi8("plain ascii 123"), "plain ascii 123");  // ASCII never changes

	EXPECT_EQ(native_text::from_koi8(koi8_privet), utf8_privet);
	// The result must be well-formed UTF-8 -- libfort aborts the process on anything else.
	EXPECT_TRUE(utf8::is_valid(native_text::from_koi8(koi8_privet)));
	EXPECT_EQ(native_text::char_count(native_text::from_koi8(koi8_privet)), 6u);
}

TEST(NativeText, ToKoi8IsTheInverseBoundary) {
	// The counterpart of from_koi8: used where something downstream speaks KOI8-R -- the legacy
	// client code pages (their tables are indexed by KOI8-R bytes) and the on-disk formats.
	const char *const koi8_privet = "\xF0\xD2\xC9\xD7\xC5\xD4";
	const char *const utf8_privet = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

	EXPECT_EQ(native_text::to_koi8(""), "");
	EXPECT_EQ(native_text::to_koi8("plain ascii 123"), "plain ascii 123");

	EXPECT_EQ(native_text::to_koi8(utf8_privet), koi8_privet);
	// Round trip through the boundary must return the original text.
	EXPECT_EQ(native_text::from_koi8(native_text::to_koi8(utf8_privet)), utf8_privet);
}

// KOI8-R spellings, so the fixture is built through from_koi8 and comes out native in either
// build. Words chosen to expose the KOI8-R byte order: there the Russian letters are NOT in
// alphabetical order, so a plain byte comparison would sort these wrong.
TEST(NativeText, SortKeyOrdersRussianAlphabetically) {
	const auto native = [](const char *koi8) { return native_text::from_koi8(koi8); };
	// "arbuz", "banan", "vishnya", "yabloko" -- a, b, v, ya.
	const std::string a = native("\xC1\xD2\xC2\xD5\xDA");
	const std::string b = native("\xC2\xC1\xCE\xC1\xCE");
	const std::string v = native("\xD7\xC9\xDB\xCE\xD1");
	const std::string ya = native("\xD1\xC2\xCC\xCF\xCB\xCF");

	EXPECT_LT(native_text::sort_key(a), native_text::sort_key(b));
	EXPECT_LT(native_text::sort_key(b), native_text::sort_key(v));
	EXPECT_LT(native_text::sort_key(v), native_text::sort_key(ya));
	// "zhaba" and "ivan": alphabetically zh comes before i, but in KOI8-R its byte (0xD6) is
	// above i's (0xC9) -- which is the whole reason the key exists.
	const std::string zh = native("\xD6\xC1\xC2\xC1");
	const std::string i = native("\xC9\xD7\xC1\xCE");
	EXPECT_LT(native_text::sort_key(zh), native_text::sort_key(i));
	// ASCII keeps its own order and stays below the Cyrillic.
	EXPECT_LT(native_text::sort_key("abc"), native_text::sort_key("abd"));
	EXPECT_LT(native_text::sort_key("zzz"), native_text::sort_key(a));
}

TEST(NativeText, SortKeyIsStableAcrossEncodings) {
	// The key is a byte string in a fixed encoding, so the very same words must produce the very
	// same key no matter which encoding the engine runs in. Pinned literally.
	EXPECT_EQ(native_text::sort_key(native_text::from_koi8("\xC1\xD2\xC2\xD5\xDA")),
			  "\xE0\xF0\xE1\xF3\xE7");   // "arbuz" in Windows-1251
	EXPECT_EQ(native_text::sort_key("plain ascii"), "plain ascii");
}

TEST(NativeText, FromDiskTextIsIdempotent) {
	// The bug this guards against: a config the engine both reads and writes was transcoded
	// unconditionally on load, so text already saved in the native encoding got transcoded a
	// second time -- and since the save wrote that back, every Cyrillic byte doubled on each
	// load/save cycle. cfg/mechanics/obj_sets.xml grew from 120 KB to 6.7 GB that way.
	const std::string koi8 = "\xD0\xD2\xC9\xD7\xC5\xD4";   // "privet" in KOI8-R
	const std::string once = native_text::from_disk_text(koi8);
	const std::string twice = native_text::from_disk_text(once);
	EXPECT_EQ(once, twice) << "reading back what we wrote must not change it";
	// However many times it goes through, the size must not grow.
	std::string acc = koi8;
	for (int i = 0; i < 10; ++i) {
		acc = native_text::from_disk_text(acc);
	}
	EXPECT_EQ(acc, once);

	EXPECT_EQ(once, "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");
	// ASCII is spelled the same in both encodings and must pass through untouched.
	EXPECT_EQ(native_text::from_disk_text("<obj vnum=\"1234\"/>"), "<obj vnum=\"1234\"/>");
	EXPECT_EQ(native_text::from_disk_text(""), "");
}

TEST(NativeText, PadRightCountsCharactersNotBytes) {
	// The replacement for printf's "%-Ns" wherever the value can be Russian: printf counts the
	// field in bytes, so a Cyrillic word ate twice its share and the column drifted (issue #3681).
	const std::string privet = native_text::from_koi8("\xD0\xD2\xC9\xD7\xC5\xD4");   // 6 letters
	const std::string padded = native_text::pad_right(privet, 10);
	EXPECT_EQ(native_text::char_count(padded), 10u) << "padding must be measured in characters";
	EXPECT_EQ(padded.substr(0, privet.size()), privet);
	EXPECT_EQ(padded.substr(privet.size()), "    ");

	// ASCII behaves exactly like "%-Ns" did.
	EXPECT_EQ(native_text::pad_right("ab", 5), "ab   ");
	EXPECT_EQ(native_text::pad_right("", 3), "   ");
	// Longer than the field: returned untouched, again like printf.
	EXPECT_EQ(native_text::pad_right("abcdef", 3), "abcdef");
	EXPECT_EQ(native_text::pad_right(privet, 2), privet);
}

TEST(NativeText, CharOffsetCutsOnCharacterBoundaries) {
	// Cutting text to fit a column by BYTES both halves the visible width under UTF-8 and can
	// split a character in two, sending a broken byte to the client (issue #3681).
	const std::string privet = native_text::from_koi8("\xD0\xD2\xC9\xD7\xC5\xD4");   // 6 letters
	const std::string cut = privet.substr(0, native_text::char_offset(privet, 3));
	EXPECT_EQ(native_text::char_count(cut), 3u);
	EXPECT_EQ(cut, privet.substr(0, 6));
	// Asking for more than there is yields the whole string, never past the end.
	EXPECT_EQ(native_text::char_offset(privet, 100), privet.size());
	EXPECT_EQ(native_text::char_offset("", 5), 0u);
	EXPECT_EQ(native_text::char_offset("abcdef", 4), 4u);
}

TEST(NativeText, ToDiskNeverTransliteratesDiskBytes) {
	// Пара к границе чтения. Если строка не проходила from_disk, она держит кои-восьмые байты,
	// и старый to_disk разбирал их как Latin-1 и гнал через словарь транслита: 'верий.свет'
	// становился 'AIEUAxAOA.OxAO' -- необратимо. Так были съедены метки вещей, сундуки дружин
	// и списки имён (issue #3681). Теперь такие байты уходят на диск как есть.
	const std::string koi8_bytes = "\xD7\xC5\xD2\xC9\xCA.\xD3\xD7\xC5\xD4";   // 'верий.свет' в KOI8-R
	EXPECT_EQ(native_text::to_disk(koi8_bytes), koi8_bytes) << "дисковые байты не должны меняться";

	// А нативный текст по-прежнему переводится в кодировку диска.
	const std::string native = native_text::from_koi8(koi8_bytes);
	EXPECT_NE(native, koi8_bytes) << "проверка построена на том, что кодировки различаются";
	EXPECT_EQ(native_text::to_disk(native), koi8_bytes);

	// Чистая латиница одинакова в обеих кодировках и не трогается ни в одном из случаев.
	EXPECT_EQ(native_text::to_disk("plain ascii"), "plain ascii");
}

TEST(NativeText, DiskRoundTripIsByteIdentical) {
	// Правило пары: что прочитано через границу, должно уйти обратно теми же байтами.
	// Нарушение этой пары -- одностороннее чтение или одностороння запись -- и съело
	// метки вещей, сундуки дружин и списки имён (issue #3681).
	const std::string on_disk = "\xD7\xC5\xD2\xC9\xCA.\xD3\xD7\xC5\xD4";   // 'верий.свет' в KOI8-R

	const std::string native = native_text::from_disk_text(on_disk);
	EXPECT_EQ(native_text::to_disk(native), on_disk) << "чтение и запись обязаны быть зеркальны";

	// Повторное чтение уже нативного текста ничего не меняет: именно на этом держится
	// идемпотентность цикла загрузка-сохранение.
	EXPECT_EQ(native_text::from_disk_text(native), native);

	// И то же самое для чистой латиницы -- она одинакова в обеих кодировках.
	const std::string ascii = "plain ascii line";
	EXPECT_EQ(native_text::to_disk(native_text::from_disk_text(ascii)), ascii);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
