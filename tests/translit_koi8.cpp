// Unit tests for the KOI8-R transliteration table (src/utils/translit_koi8.*, issue #3681).
//
// Pure ASCII: every non-ASCII fixture is spelled as UTF-8 byte escapes, so the test data does not
// depend on how an editor happens to save this file.

#include "utils/translit_koi8.h"
#include "utils/native_text.h"
#include "utils/utf8.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using codepages::TranslitToKoi8;

namespace {

// Convenience: the replacement as a std::string, or "<none>" so a failure prints readably.
std::string Tr(char32_t cp) {
	const char *const r = TranslitToKoi8(cp);
	return r == nullptr ? std::string("<none>") : std::string(r);
}

}  // namespace

TEST(TranslitKoi8, TypographyBecomesPlainAscii) {
	EXPECT_EQ(Tr(0x2014), "-");      // EM DASH
	EXPECT_EQ(Tr(0x2013), "-");      // EN DASH
	EXPECT_EQ(Tr(0x2026), "...");    // HORIZONTAL ELLIPSIS
	EXPECT_EQ(Tr(0x00AB), "\"");     // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
	EXPECT_EQ(Tr(0x00BB), "\"");     // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
	EXPECT_EQ(Tr(0x201C), "\"");     // LEFT DOUBLE QUOTATION MARK
	EXPECT_EQ(Tr(0x2019), "'");      // RIGHT SINGLE QUOTATION MARK
	EXPECT_EQ(Tr(0x2122), "tm");     // TRADE MARK SIGN
	EXPECT_EQ(Tr(0x00AD), "");       // SOFT HYPHEN: drops out entirely
	// A no-break space needs no replacement: KOI8-R has one of its own (0x9A).
	EXPECT_EQ(TranslitToKoi8(0x00A0), nullptr);
}

TEST(TranslitKoi8, AccentedLatinLosesItsAccent) {
	EXPECT_EQ(Tr(0x00E9), "e");   // e with acute
	EXPECT_EQ(Tr(0x00FC), "u");   // u with diaeresis
	EXPECT_EQ(Tr(0x00C0), "A");   // A with grave: case is kept
	EXPECT_EQ(Tr(0x0141), "L");   // L with stroke (no NFKD decomposition; entered by hand)
}

TEST(TranslitKoi8, NonRussianCyrillicMapsToItsRussianNeighbour) {
	EXPECT_EQ(Tr(0x0404), "\xD0\xAD");   // Ukrainian Ye -> Russian E
	EXPECT_EQ(Tr(0x0456), "\xD0\xB8");   // Ukrainian i -> Russian i
	EXPECT_EQ(Tr(0x0490), "\xD0\x93");   // Ukrainian Ghe with upturn -> Russian Ghe
}

TEST(TranslitKoi8, WhatKoi8AlreadyHasIsNotTouched) {
	// ASCII and the whole Russian alphabet are representable, so there is nothing to replace
	// and the table must stay out of the way.
	for (char32_t cp = 0x20; cp < 0x7F; ++cp) {
		EXPECT_EQ(TranslitToKoi8(cp), nullptr) << "ASCII " << static_cast<unsigned>(cp);
	}
	for (char32_t cp = 0x0410; cp <= 0x044F; ++cp) {
		EXPECT_EQ(TranslitToKoi8(cp), nullptr) << "Cyrillic U+" << static_cast<unsigned>(cp);
	}
	EXPECT_EQ(TranslitToKoi8(0x0401), nullptr);   // Yo
	EXPECT_EQ(TranslitToKoi8(0x0451), nullptr);   // yo
	EXPECT_EQ(TranslitToKoi8(0x2500), nullptr);   // box drawing: KOI8-R has these
}

TEST(TranslitKoi8, NoEquivalentYieldsNullptr) {
	EXPECT_EQ(TranslitToKoi8(0x1F600), nullptr);   // grinning face
	EXPECT_EQ(TranslitToKoi8(0x4E00), nullptr);    // CJK ideograph
	EXPECT_EQ(TranslitToKoi8(0x05D0), nullptr);    // Hebrew alef
}

TEST(TranslitKoi8, TableIsSortedAndUnique) {
	// The lookup is a binary search; a table out of order would silently miss entries.
	char32_t previous = 0;
	unsigned found = 0;
	for (char32_t cp = 1; cp <= 0x2FFF; ++cp) {
		if (TranslitToKoi8(cp) != nullptr) {
			EXPECT_LT(previous, cp);
			previous = cp;
			++found;
		}
	}
	EXPECT_GT(found, 200u) << "the generated table looks truncated";
}

TEST(TranslitKoi8, ToKoi8AppliesTheTable) {
	// "-- privet ..." with an em dash and an ellipsis around Cyrillic text.
	const std::string source =
		"\xE2\x80\x94" " \xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82" "\xE2\x80\xA6";
	const std::string koi = native_text::to_koi8(source);
	EXPECT_EQ(koi.front(), '-');
	EXPECT_EQ(koi.substr(koi.size() - 3), "...");
	// The Cyrillic in the middle survived as KOI8-R "privet".
	EXPECT_EQ(koi, "- \xD0\xD2\xC9\xD7\xC5\xD4...");
}

TEST(TranslitKoi8, ToKoi8FallsBackToThePlaceholder) {
	// An emoji has no KOI8-R equivalent at all, so it becomes the single placeholder character.
	EXPECT_EQ(native_text::to_koi8("a\xF0\x9F\x98\x80" "b"), "a?b");
}

TEST(TranslitKoi8, GraphicsDegradeToWhatKoi8CanDraw) {
	// Frames: the rounded and heavy corners reduce to the plain ones KOI8-R does have.
	EXPECT_EQ(Tr(0x256D), "\xE2\x94\x8C");   // arc down and right -> U+250C
	EXPECT_EQ(Tr(0x256E), "\xE2\x94\x90");   // arc down and left  -> U+2510
	EXPECT_EQ(Tr(0x256F), "\xE2\x94\x98");   // arc up and left    -> U+2518
	EXPECT_EQ(Tr(0x2570), "\xE2\x94\x94");   // arc up and right   -> U+2514
	EXPECT_EQ(Tr(0x2501), "\xE2\x94\x80");   // heavy horizontal   -> U+2500
	EXPECT_EQ(Tr(0x2503), "\xE2\x94\x82");   // heavy vertical     -> U+2502
	// Partial blocks and stars reduce to something that still draws.
	EXPECT_EQ(Tr(0x2589), "\xE2\x96\x88");   // 7/8 block -> full block
	EXPECT_EQ(Tr(0x2726), "*");                // black four pointed star
	EXPECT_EQ(Tr(0x2605), "*");                // black star
}

TEST(TranslitKoi8, ReplacementNeverGrowsTheText) {
	// The substitution is a pre-pass over the UTF-8, and callers rely on the conversion to
	// KOI8-R never making the string longer. So a replacement must fit in the bytes the
	// character itself occupied.
	for (char32_t cp = 1; cp <= 0x10FFFF; ++cp) {
		if (cp >= 0xD800 && cp <= 0xDFFF) {
			continue;   // surrogates are not encodable
		}
		const char *const r = TranslitToKoi8(cp);
		if (r == nullptr) {
			continue;
		}
		std::string source;
		ASSERT_GT(utf8::encode(cp, source), 0u) << "U+" << static_cast<unsigned>(cp);
		EXPECT_LE(std::strlen(r), source.size())
			<< "replacement for U+" << static_cast<unsigned>(cp) << " is longer than the character";
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
