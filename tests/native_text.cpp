// Unit tests for the native-encoding character helpers (src/utils/native_text.*, issue #3681).
//
// Pure ASCII: non-ASCII fixtures are spelled as UTF-8 byte escapes. Expectations that differ
// between the KOI8-R and UTF-8 builds branch on native_text::native_is_utf8() (which reflects the
// flag the library was built with), so this one test file is correct under either build.

#include "utils/native_text.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string_view>

namespace {

// "Privet": 6 Cyrillic code points, 12 UTF-8 bytes.
const char *const kPrivet = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

}  // namespace

TEST(NativeText, CharCountAscii) {
	EXPECT_EQ(native_text::char_count("Hello"), 5u);
	EXPECT_EQ(native_text::char_count(kPrivet, kPrivet + 4), native_text::native_is_utf8() ? 2u : 4u);
}

TEST(NativeText, CharCountReflectsEncoding) {
	if (native_text::native_is_utf8()) {
		EXPECT_EQ(native_text::char_count(kPrivet), 6u);
		EXPECT_EQ(native_text::char_count(kPrivet, kPrivet + 12), 6u);
	} else {
		EXPECT_EQ(native_text::char_count(kPrivet), 12u);
		EXPECT_EQ(native_text::char_count(kPrivet, kPrivet + 12), 12u);
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

TEST(NativeText, TruncateOffset) {
	const std::string_view p(kPrivet, 12);
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
