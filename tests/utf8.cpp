// Unit tests for the character-semantic UTF-8 helpers (src/utils/utf8.*, issue #3681).
//
// This file is intentionally pure ASCII: every non-ASCII string is spelled out as explicit
// UTF-8 byte escapes so the test data is independent of the source file's ambient encoding
// (which is KOI8-R today and UTF-8 after the migration flip). Adjacent string literals are
// concatenated so an \xNN escape is never followed by a literal hex digit.

#include "utils/utf8.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

namespace {

// "Privet" (Cyrillic) -- 6 letters, 12 bytes.
const char *const kPrivet = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
// "PRIVET" (all upper) and "privet" (all lower).
const char *const kPrivetUpper = "\xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2";
const char *const kPrivetLower = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

std::string S(std::string_view v) {
	return std::string(v);
}

}  // namespace

TEST(Utf8, EmptyString) {
	EXPECT_EQ(utf8::length(""), 0u);
	EXPECT_TRUE(utf8::is_valid(""));
	EXPECT_EQ(utf8::substr("", 0), "");
	EXPECT_EQ(utf8::substr("", 3, 5), "");
	EXPECT_EQ(S(utf8::char_at("", 0)), "");
	EXPECT_EQ(utf8::to_lower(std::string_view("")), "");
	EXPECT_EQ(utf8::byte_offset("", 4), 0u);
}

TEST(Utf8, AsciiSemanticsMatchBytes) {
	EXPECT_EQ(utf8::length("Hello"), 5u);
	EXPECT_TRUE(utf8::is_valid("Hello, world!"));
	EXPECT_EQ(utf8::to_lower(std::string_view("HeLLo")), "hello");
	EXPECT_EQ(utf8::to_upper(std::string_view("HeLLo")), "HELLO");
	EXPECT_EQ(utf8::substr("Hello", 1, 3), "ell");
	EXPECT_EQ(S(utf8::char_at("Hello", 1)), "e");
	EXPECT_EQ(S(utf8::char_at("Hello", 5)), "");
}

TEST(Utf8, CyrillicLengthIsCodePointsNotBytes) {
	EXPECT_EQ(std::strlen(kPrivet), 12u);  // sanity: the fixture really is 12 bytes
	EXPECT_EQ(utf8::length(kPrivet), 6u);
	EXPECT_TRUE(utf8::is_valid(kPrivet));
}

TEST(Utf8, CyrillicCaseFolding) {
	EXPECT_EQ(utf8::to_lower(std::string_view(kPrivetUpper)), kPrivetLower);
	EXPECT_EQ(utf8::to_upper(std::string_view(kPrivetLower)), kPrivetUpper);
	// "Privet, Mir" -> "privet, mir" (mixed Cyrillic + ASCII punctuation).
	const char *const mixed = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82" ", " "\xD0\x9C\xD0\xB8\xD1\x80";
	const char *const mixed_lower = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82" ", " "\xD0\xBC\xD0\xB8\xD1\x80";
	EXPECT_EQ(utf8::to_lower(std::string_view(mixed)), mixed_lower);
}

TEST(Utf8, YoLetter) {
	const char *const kYoUpper = "\xD0\x81";  // U+0401 (Yo)
	const char *const kYoLower = "\xD1\x91";  // U+0451 (yo)
	EXPECT_EQ(utf8::length(kYoUpper), 1u);
	EXPECT_EQ(std::strlen(kYoUpper), 2u);
	EXPECT_EQ(utf8::to_lower(std::string_view(kYoUpper)), kYoLower);
	EXPECT_EQ(utf8::to_upper(std::string_view(kYoLower)), kYoUpper);
	EXPECT_EQ(utf8::to_lower(0x0401u), 0x0451u);
	EXPECT_EQ(utf8::to_upper(0x0451u), 0x0401u);
}

TEST(Utf8, SubstrAndIndexOnCyrillic) {
	// chars: [0]P [1]r [2]i [3]v [4]e [5]t
	EXPECT_EQ(utf8::substr(kPrivet, 1, 3), "\xD1\x80\xD0\xB8\xD0\xB2");  // "riv" (chars 1..3)
	EXPECT_EQ(utf8::substr(kPrivet, 4), "\xD0\xB5\xD1\x82");             // "et" to end
	EXPECT_EQ(utf8::substr(kPrivet, 10), "");                            // pos past end
	EXPECT_EQ(S(utf8::char_at(kPrivet, 0)), "\xD0\x9F");                 // char "P"
	EXPECT_EQ(S(utf8::char_at(kPrivet, 5)), "\xD1\x82");                 // char "t"
	EXPECT_EQ(S(utf8::char_at(kPrivet, 6)), "");                         // out of range
	EXPECT_EQ(utf8::byte_offset(kPrivet, 2), 4u);
	EXPECT_EQ(utf8::byte_offset(kPrivet, 6), 12u);
	EXPECT_EQ(utf8::byte_offset(kPrivet, 100), 12u);
}

TEST(Utf8, FourByteAndBom) {
	const char *const kGrin = "\xF0\x9F\x98\x80";  // U+1F600
	EXPECT_EQ(utf8::length(kGrin), 1u);
	EXPECT_EQ(std::strlen(kGrin), 4u);
	EXPECT_TRUE(utf8::is_valid(kGrin));
	EXPECT_EQ(S(utf8::char_at(kGrin, 0)), kGrin);
	EXPECT_EQ(utf8::sequence_length(0xF0), 4);

	const char *const kBom = "\xEF\xBB\xBF";  // U+FEFF
	EXPECT_EQ(utf8::length(kBom), 1u);
	EXPECT_TRUE(utf8::is_valid(kBom));
	char32_t cp = 0;
	EXPECT_EQ(utf8::decode(kBom, 0, cp), 3u);
	EXPECT_EQ(cp, 0xFEFFu);
}

TEST(Utf8, SequenceLength) {
	EXPECT_EQ(utf8::sequence_length(0x41), 1);  // 'A'
	EXPECT_EQ(utf8::sequence_length(0xD0), 2);
	EXPECT_EQ(utf8::sequence_length(0xE0), 3);
	EXPECT_EQ(utf8::sequence_length(0xF0), 4);
	EXPECT_EQ(utf8::sequence_length(0x80), 1);  // stray continuation
	EXPECT_EQ(utf8::sequence_length(0xFF), 1);  // out-of-range lead
}

TEST(Utf8, DecodeBoundaries) {
	char32_t cp = 0xABCD;
	EXPECT_EQ(utf8::decode("", 0, cp), 0u);   // empty
	EXPECT_EQ(cp, 0u);
	EXPECT_EQ(utf8::decode("A", 1, cp), 0u);  // pos at end
	EXPECT_EQ(utf8::decode("A", 0, cp), 1u);
	EXPECT_EQ(cp, static_cast<char32_t>('A'));
}

TEST(Utf8, EncodeRoundTrip) {
	std::string out;
	EXPECT_EQ(utf8::encode('A', out), 1u);
	EXPECT_EQ(out, "A");
	out.clear();
	EXPECT_EQ(utf8::encode(0x041Fu, out), 2u);  // U+041F (P)
	EXPECT_EQ(out, "\xD0\x9F");
	out.clear();
	EXPECT_EQ(utf8::encode(0x1F600u, out), 4u);  // U+1F600
	EXPECT_EQ(out, "\xF0\x9F\x98\x80");
	out.clear();
	EXPECT_EQ(utf8::encode(0xD800u, out), 0u);  // surrogate rejected
	EXPECT_TRUE(out.empty());
	EXPECT_EQ(utf8::encode(0x110000u, out), 0u);  // above U+10FFFF
	EXPECT_TRUE(out.empty());
}

TEST(Utf8, RejectsMalformed) {
	EXPECT_FALSE(utf8::is_valid("\x80"));              // lone continuation
	EXPECT_FALSE(utf8::is_valid("\xD0"));              // truncated 2-byte
	EXPECT_FALSE(utf8::is_valid("Hi\xD0"));            // truncated at end
	EXPECT_FALSE(utf8::is_valid("\xC0\x80"));          // overlong NUL (0xC0 lead)
	EXPECT_FALSE(utf8::is_valid("\xC0\xAF"));          // overlong '/'
	EXPECT_FALSE(utf8::is_valid("\xE0\x80\xAF"));      // overlong 3-byte
	EXPECT_FALSE(utf8::is_valid("\xED\xA0\x80"));      // U+D800 surrogate
	EXPECT_FALSE(utf8::is_valid("\xF4\x90\x80\x80"));  // U+110000, above range
	EXPECT_FALSE(utf8::is_valid("\xF5\x80\x80\x80"));  // 0xF5 lead
}

TEST(Utf8, AcceptsRangeEdges) {
	EXPECT_TRUE(utf8::is_valid("\xED\x9F\xBF"));       // U+D7FF, just below surrogates
	EXPECT_TRUE(utf8::is_valid("\xEE\x80\x80"));       // U+E000, just above surrogates
	EXPECT_TRUE(utf8::is_valid("\xF4\x8F\xBF\xBF"));   // U+10FFFF, top of range
	EXPECT_TRUE(utf8::is_valid("\xC2\x80"));           // U+0080, smallest 2-byte
}

TEST(Utf8, LenientCountingAndFolding) {
	// Malformed bytes are counted as one code point each and passed through by the folders,
	// so nothing is dropped when the helpers meet non-UTF-8 (e.g. legacy KOI8-R) data.
	EXPECT_EQ(utf8::length("\x80\x80"), 2u);
	EXPECT_EQ(utf8::to_lower(std::string_view("\x80\x80")), "\x80\x80");
	EXPECT_EQ(utf8::to_upper(std::string_view("A\xFF" "Z")), "A\xFF" "Z");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
