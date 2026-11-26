// Part of Bylins http://www.mud.ru
// Ревизия мира читается при старте из каталога мира, а не впечатывается в бинарь
// (мир -- отдельный репозиторий, выкладывается независимо от сборки). Здесь
// проверяется именно чтение: git мы разбираем сами, двумя файлами, поэтому
// упакованная ссылка, отцепленный HEAD и запасной файл REVISION -- это три разных
// пути, и каждый должен отдавать короткий хеш, а не мусор.

// gtest first: on clang-cl (Windows) the project headers pull conf.h/sysdep.h
// ahead of the system headers, which breaks the MSVC/clang intrinsic headers.
#include <gtest/gtest.h>

#include "engine/db/world_revision.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

const std::string kSha = "b047e9a224c0ffee1234567890abcdef12345678";

// Своя пустая папка на каждый случай: тесты идут в одном процессе, и ревизия
// живёт в статике, поэтому каждый Load должен полностью перекрывать прошлый.
class WorldRevision : public ::testing::Test {
 protected:
	void SetUp() override {
		dir = fs::temp_directory_path() / fs::path("bylins_world_revision_test_" +
			std::to_string(reinterpret_cast<uintptr_t>(this)));
		fs::remove_all(dir);
		fs::create_directories(dir);
	}
	void TearDown() override { fs::remove_all(dir); }

	void Write(const fs::path &rel, const std::string &text) const {
		const fs::path path = dir / rel;
		fs::create_directories(path.parent_path());
		std::ofstream file(path);
		file << text;
	}

	fs::path dir;
};

TEST_F(WorldRevision, ReadsLooseRef) {
	Write(".git/HEAD", "ref: refs/heads/master\n");
	Write(".git/refs/heads/master", kSha + "\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22 (master)", world_revision::Get());
}

TEST_F(WorldRevision, ReadsPackedRef) {
	// У репозитория мира ссылки как раз упакованы, файла refs/heads/master нет.
	Write(".git/HEAD", "ref: refs/heads/master\n");
	Write(".git/packed-refs",
		  "# pack-refs with: peeled fully-peeled sorted \n"
		  "0000000000000000000000000000000000000000 refs/heads/other\n"
		  + kSha + " refs/heads/master\n"
		  "1111111111111111111111111111111111111111 refs/tags/v1\n"
		  "^2222222222222222222222222222222222222222\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22 (master)", world_revision::Get());
}

TEST_F(WorldRevision, ReadsDetachedHead) {
	Write(".git/HEAD", kSha + "\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22", world_revision::Get());
}

TEST_F(WorldRevision, FollowsGitdirFileOfAWorktree) {
	// В worktree ".git" -- не каталог, а файл со ссылкой наружу.
	Write("real_git/HEAD", "ref: refs/heads/work\n");
	Write("real_git/refs/heads/work", kSha + "\n");
	Write(".git", "gitdir: " + (dir / "real_git").string() + "\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22 (work)", world_revision::Get());
}

TEST_F(WorldRevision, FallsBackToRevisionFileWithoutGit) {
	Write("REVISION", "deploy-2026-11-26\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("deploy-2026-11-26", world_revision::Get());
}

TEST_F(WorldRevision, PrefersGitOverRevisionFile) {
	Write(".git/HEAD", kSha + "\n");
	Write("REVISION", "stale-stamp\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22", world_revision::Get());
}

TEST_F(WorldRevision, EmptyWhenNothingToRead) {
	world_revision::Load(dir.string());
	EXPECT_TRUE(world_revision::Get().empty());
}

TEST_F(WorldRevision, DropsColourCodesAndNonPrintableFromStamp) {
	// Строка из файла уходит игроку в "ver": '&' -- код цвета, и чужая строка не
	// должна красить чужой вывод.
	Write("REVISION", "tag&Rred\x07\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("tagRred", world_revision::Get());
}

TEST_F(WorldRevision, IgnoresGarbageInsteadOfShowingIt) {
	Write(".git/HEAD", "ref: refs/heads/master\n");
	Write(".git/refs/heads/master", "not-a-sha\n");
	world_revision::Load(dir.string());
	EXPECT_TRUE(world_revision::Get().empty());
}

TEST(WorldRevisionEdits, ListsZoneOncePerKindInVnumOrder) {
	// Вноситься зона может несколько раз (комнаты и мобы пишутся отдельно), а в
	// списке должна быть одной записью.
	world_revision::NoteZoneSaved(90002, "мобы");
	world_revision::NoteZoneSaved(90001, "комнаты");
	world_revision::NoteZoneSaved(90001, "мобы");
	world_revision::NoteZoneSaved(90001, "мобы");
	const std::string edits = world_revision::FormatEdits();
	EXPECT_NE(std::string::npos, edits.find("90001 (комнаты+мобы, "));
	EXPECT_NE(std::string::npos, edits.find("90002 (мобы, "));
	EXPECT_LT(edits.find("90001"), edits.find("90002"));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
