// issue.misc-migrate: line-I/O API of state::StateManager (LoadLines / SaveLines / AppendLine /
// RewriteDropping). Exercises a throwaway state file under the test working dir.

#include "engine/boot/state_manager.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {
using state::EStateFile;
using Lines = std::vector<std::string>;

// A state file the test owns end to end (created and removed here).
constexpr EStateFile kFile = EStateFile::kBannedProxies;
}  // namespace

TEST(StateManagerIO, LoadSaveAppendRewrite) {
	state::StateManager sm;
	std::filesystem::create_directories("state");
	const std::string &path = sm.Path(kFile);
	std::filesystem::remove(path);

	// A missing file reads as an empty list.
	EXPECT_TRUE(sm.LoadLines(kFile).empty());

	// SaveLines round-trips through the file (atomic replace).
	ASSERT_TRUE(sm.SaveLines(kFile, Lines{"alpha", "beta", "gamma"}));
	EXPECT_EQ(sm.LoadLines(kFile), (Lines{"alpha", "beta", "gamma"}));

	// SaveLines must OVERWRITE an existing destination: the tmp+rename step has to
	// replace the file that is already there. std::rename() fails at exactly this
	// point on Windows, so this guards that cross-platform regression.
	ASSERT_TRUE(sm.SaveLines(kFile, Lines{"alpha", "beta", "gamma"}));
	EXPECT_EQ(sm.LoadLines(kFile), (Lines{"alpha", "beta", "gamma"}));

	// AppendLine adds one record.
	ASSERT_TRUE(sm.AppendLine(kFile, "delta"));
	EXPECT_EQ(sm.LoadLines(kFile), (Lines{"alpha", "beta", "gamma", "delta"}));

	// RewriteDropping removes matching records and keeps order.
	ASSERT_TRUE(sm.RewriteDropping(kFile, [](const std::string &l) { return l == "beta"; }));
	EXPECT_EQ(sm.LoadLines(kFile), (Lines{"alpha", "gamma", "delta"}));

	std::filesystem::remove(path);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
