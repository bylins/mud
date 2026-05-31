// Part of Bylins http://www.mud.ru
// Tests for the buffer-based FileCRC API (update/verify/reset) that replaces
// the old "write file, then read it back to compute CRC" flow.

#include "utils/file_crc.h"

#include <gtest/gtest.h>

#include <string>

namespace {

// Each test uses a distinct uid: crc_list is process-global and shared across
// tests, so unique uids keep them independent.

TEST(FileCrcBuffer, UpdateThenVerifyMatches) {
	const long uid = 7700001;
	const std::string a = "hello world\n$\n$\n";
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TEXTOBJS, a.data(), a.size());
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::TEXTOBJS, a.data(), a.size()));
}

TEST(FileCrcBuffer, VerifyDetectsMismatch) {
	const long uid = 7700002;
	const std::string a = "alpha";
	const std::string b = "beta!";
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TIMEOBJS, a.data(), a.size());
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::TIMEOBJS, b.data(), b.size()));
}

TEST(FileCrcBuffer, ResetZeroesCrc) {
	const long uid = 7700003;
	const std::string a = "some object data";
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TEXTOBJS, a.data(), a.size());
	ASSERT_TRUE(FileCRC::verify_from_content(uid, FileCRC::TEXTOBJS, a.data(), a.size()));
	FileCRC::reset(uid, FileCRC::TEXTOBJS);
	// stored crc is now 0; the buffer's crc is non-zero -> mismatch
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::TEXTOBJS, a.data(), a.size()));
}

TEST(FileCrcBuffer, VerifyWithoutSnapshotEstablishesBaseline) {
	const long uid = 7700004;
	const std::string a = "baseline content";
	// No prior snapshot: verify must accept it and remember it as the baseline.
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::PLAYER, a.data(), a.size()));
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::PLAYER, a.data(), a.size()));
}

TEST(FileCrcBuffer, FileTypesAreIndependent) {
	const long uid = 7700005;
	const std::string text = "text objects";
	const std::string time = "time objects";
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TEXTOBJS, text.data(), text.size());
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TIMEOBJS, time.data(), time.size());
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::TEXTOBJS, text.data(), text.size()));
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::TIMEOBJS, time.data(), time.size()));
	// text content checked against the timeobjs slot must not match
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::TIMEOBJS, text.data(), text.size()));
}

TEST(FileCrcBuffer, EmptyBufferIsStable) {
	const long uid = 7700006;
	FileCRC::update_from_content(uid, FileCRC::UPDATE_TEXTOBJS, "", 0);
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::TEXTOBJS, "", 0));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
