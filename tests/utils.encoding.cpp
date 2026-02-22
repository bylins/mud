#include "utils/utils.h"

#include <gtest/gtest.h>

#include <cstring>

// Helper to convert UTF-8 string to KOI8-R and return as std::string
std::string Utf8ToKoi(const char* utf8_input)
{
	char output[1024] = {0};
	utf8_to_koi(const_cast<char*>(utf8_input), output);
	return std::string(output);
}

// Helper to create a string from hex bytes
std::string HexToString(const std::initializer_list<unsigned char>& bytes)
{
	return std::string(bytes.begin(), bytes.end());
}

TEST(Utils_Encoding, Utf8ToKoi_EmptyString)
{
	EXPECT_EQ("", Utf8ToKoi(""));
}

TEST(Utils_Encoding, Utf8ToKoi_AsciiPassthrough)
{
	EXPECT_EQ("Hello World", Utf8ToKoi("Hello World"));
	EXPECT_EQ("123 abc XYZ", Utf8ToKoi("123 abc XYZ"));
	EXPECT_EQ("!@#$%^&*()", Utf8ToKoi("!@#$%^&*()"));
}

TEST(Utils_Encoding, Utf8ToKoi_CyrillicUppercase)
{
	// UTF-8: D090 = A (U+0410), D091 = B (U+0411), etc.
	// KOI8-R: A = E1, B = E2, etc.

	// Test "ABC" in Cyrillic (U+0410 U+0411 U+0412)
	// UTF-8: D0 90 D0 91 D0 92
	std::string utf8_abc = "\xD0\x90\xD0\x91\xD0\x92";
	std::string koi_abc = Utf8ToKoi(utf8_abc.c_str());

	// KOI8-R uppercase A=E1, B=E2, V=F7
	EXPECT_EQ(HexToString({0xE1, 0xE2, 0xF7}), koi_abc);
}

TEST(Utils_Encoding, Utf8ToKoi_CyrillicLowercase)
{
	// Test lowercase Cyrillic
	// UTF-8: D0 B0 D0 B1 D0 B2 (a b v)
	std::string utf8_abc = "\xD0\xB0\xD0\xB1\xD0\xB2";
	std::string koi_abc = Utf8ToKoi(utf8_abc.c_str());

	// KOI8-R lowercase a=C1, b=C2, v=D7
	EXPECT_EQ(HexToString({0xC1, 0xC2, 0xD7}), koi_abc);
}

TEST(Utils_Encoding, Utf8ToKoi_NoBreakSpace)
{
	// NO-BREAK SPACE: U+00A0 = UTF-8 C2 A0 = KOI8-R 9A
	std::string utf8_nbsp = "\xC2\xA0";
	std::string koi_nbsp = Utf8ToKoi(utf8_nbsp.c_str());
	EXPECT_EQ(HexToString({0x9A}), koi_nbsp);

	// Multiple NO-BREAK SPACEs
	std::string utf8_3nbsp = "\xC2\xA0\xC2\xA0\xC2\xA0";
	std::string koi_3nbsp = Utf8ToKoi(utf8_3nbsp.c_str());
	EXPECT_EQ(HexToString({0x9A, 0x9A, 0x9A}), koi_3nbsp);
}

TEST(Utils_Encoding, Utf8ToKoi_DegreeSign)
{
	// Degree sign: U+00B0 = UTF-8 C2 B0 = KOI8-R 9C
	std::string utf8_deg = "\xC2\xB0";
	std::string koi_deg = Utf8ToKoi(utf8_deg.c_str());
	EXPECT_EQ(HexToString({0x9C}), koi_deg);
}

TEST(Utils_Encoding, Utf8ToKoi_CopyrightSign)
{
	// Copyright: U+00A9 = UTF-8 C2 A9 = KOI8-R BF
	std::string utf8_copy = "\xC2\xA9";
	std::string koi_copy = Utf8ToKoi(utf8_copy.c_str());
	EXPECT_EQ(HexToString({0xBF}), koi_copy);
}

TEST(Utils_Encoding, Utf8ToKoi_MixedContent)
{
	// Mix of ASCII, Cyrillic, and special chars
	// "Hi " + Cyrillic A + " " + NO-BREAK SPACE + "!"
	std::string utf8_mixed = "Hi \xD0\x90 \xC2\xA0!";
	std::string koi_mixed = Utf8ToKoi(utf8_mixed.c_str());

	// Expected: "Hi " + E1 (KOI A) + " " + 9A (NBSP) + "!"
	std::string expected = "Hi ";
	expected += (char)0xE1;
	expected += " ";
	expected += (char)0x9A;
	expected += "!";
	EXPECT_EQ(expected, koi_mixed);
}

TEST(Utils_Encoding, Utf8ToKoi_YoUppercase)
{
	// Yo uppercase: U+0401 = UTF-8 D0 81 = KOI8-R B3
	std::string utf8_yo = "\xD0\x81";
	std::string koi_yo = Utf8ToKoi(utf8_yo.c_str());
	EXPECT_EQ(HexToString({0xB3}), koi_yo);
}

TEST(Utils_Encoding, Utf8ToKoi_YoLowercase)
{
	// Yo lowercase: U+0451 = UTF-8 D1 91 = KOI8-R A3
	std::string utf8_yo = "\xD1\x91";
	std::string koi_yo = Utf8ToKoi(utf8_yo.c_str());
	EXPECT_EQ(HexToString({0xA3}), koi_yo);
}

TEST(Utils_Encoding, Utf8ToKoi_UnknownCharReplacement)
{
	// Characters not in KOI8-R should be replaced with KOI8_UNKNOWN_CHAR (+)
	// For example, Euro sign: U+20AC = UTF-8 E2 82 AC
	std::string utf8_euro = "\xE2\x82\xAC";
	std::string koi_euro = Utf8ToKoi(utf8_euro.c_str());
	EXPECT_EQ("+", koi_euro);
}

TEST(Utils_Encoding, Utf8ToKoi_BoxDrawingChars)
{
	// Box drawing: U+2550 = UTF-8 E2 95 90 = KOI8-R A0
	std::string utf8_box = "\xE2\x95\x90";
	std::string koi_box = Utf8ToKoi(utf8_box.c_str());
	EXPECT_EQ(HexToString({0xA0}), koi_box);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
