// Part of Bylins http://www.mud.ru
// Tests for the buffer-based FileCRC API (update/verify/reset) that replaces
// the old "write file, then read it back to compute CRC" flow.

// gtest first: on clang-cl (Windows) the project headers pull conf.h/sysdep.h
// ahead of the system headers, which breaks the MSVC/clang intrinsic headers.
#include <gtest/gtest.h>

#include "utils/file_crc.h"

#include <string>

namespace {

// Each test uses a distinct uid: crc_list is process-global and shared across
// tests, so unique uids keep them independent.

TEST(FileCrcBuffer, UpdateThenVerifyMatches) {
	const long uid = 7700001;
	const std::string a = "hello world\n$\n$\n";
	FileCRC::update_from_content(uid, FileCRC::kTextObjs, a.data(), a.size());
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kTextObjs, a.data(), a.size()));
}

TEST(FileCrcBuffer, VerifyDetectsMismatch) {
	const long uid = 7700002;
	const std::string a = "alpha";
	const std::string b = "beta!";
	FileCRC::update_from_content(uid, FileCRC::kTimeObjs, a.data(), a.size());
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::kTimeObjs, b.data(), b.size()));
}

TEST(FileCrcBuffer, ResetZeroesCrc) {
	const long uid = 7700003;
	const std::string a = "some object data";
	FileCRC::update_from_content(uid, FileCRC::kTextObjs, a.data(), a.size());
	ASSERT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kTextObjs, a.data(), a.size()));
	FileCRC::reset(uid, FileCRC::kTextObjs);
	// stored crc is now 0; the buffer's crc is non-zero -> mismatch
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::kTextObjs, a.data(), a.size()));
}

TEST(FileCrcBuffer, VerifyWithoutSnapshotEstablishesBaseline) {
	const long uid = 7700004;
	const std::string a = "baseline content";
	// No prior snapshot: verify must accept it and remember it as the baseline.
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kPlayer, a.data(), a.size()));
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kPlayer, a.data(), a.size()));
}

TEST(FileCrcBuffer, FileTypesAreIndependent) {
	const long uid = 7700005;
	const std::string text = "text objects";
	const std::string time = "time objects";
	FileCRC::update_from_content(uid, FileCRC::kTextObjs, text.data(), text.size());
	FileCRC::update_from_content(uid, FileCRC::kTimeObjs, time.data(), time.size());
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kTextObjs, text.data(), text.size()));
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kTimeObjs, time.data(), time.size()));
	// text content checked against the timeobjs slot must not match
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::kTimeObjs, text.data(), text.size()));
}

TEST(FileCrcBuffer, EmptyBufferIsStable) {
	const long uid = 7700006;
	FileCRC::update_from_content(uid, FileCRC::kTextObjs, "", 0);
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kTextObjs, "", 0));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
