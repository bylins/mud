// Part of Bylins http://www.mud.ru
// Ревизия мира читается при старте из каталога мира, а не впечатывается в бинарь
// (мир -- отдельный репозиторий, выкладывается независимо от сборки). Здесь
// проверяется именно чтение: git мы разбираем сами, файлами, поэтому упакованная
// ссылка, отцепленный HEAD, reflog и запасной файл REVISION -- это разные пути, и
// каждый должен отдавать либо короткий хеш с датой, либо ничего, но не мусор.
// Плюс список правленных зон: он лежит в state/, переживает перезагрузки и
// обнуляется, когда выложена другая ревизия.

// gtest first: on clang-cl (Windows) the project headers pull conf.h/sysdep.h
// ahead of the system headers, which breaks the MSVC/clang intrinsic headers.
#include <gtest/gtest.h>

#include "engine/db/world_revision.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

const std::string kSha = "b047e9a224c0ffee1234567890abcdef12345678";
const std::string kOtherSha = "3e0febd1ec9f459093a191571e93ca745f1f1109";
const std::string kZeroSha = "0000000000000000000000000000000000000000";

// Как ту же метку времени напечатал бы сам модуль: часовой пояс машины, на
// которой идут тесты, заранее не известен, поэтому ждём не строку, а согласие.
std::string LocalDate(std::time_t when) {
	struct tm broken {};
#ifdef _WIN32
	localtime_s(&broken, &when);
#else
	localtime_r(&when, &broken);
#endif
	char out[64];
	const std::size_t len = std::strftime(out, sizeof(out), "%d.%m.%Y %H:%M", &broken);
	return std::string(out, len);
}

// Своя пустая папка на каждый случай: тесты идут в одном процессе, и ревизия со
// списком живут в статике, поэтому каждый Load должен полностью перекрывать прошлый.
class WorldRevision : public ::testing::Test {
 protected:
	void SetUp() override {
		dir = fs::temp_directory_path() / fs::path("bylins_world_revision_test_" +
			std::to_string(reinterpret_cast<uintptr_t>(this)));
		fs::remove_all(dir);
		fs::create_directories(dir);
	}
	void TearDown() override {
		// Хвост от предыдущего случая не должен уехать в следующий.
		world_revision::Load(dir.string(), "");
		fs::remove_all(dir);
	}

	void Write(const fs::path &rel, const std::string &text) const {
		const fs::path path = dir / rel;
		fs::create_directories(path.parent_path());
		std::ofstream file(path);
		file << text;
	}

	[[nodiscard]] std::string StatePath() const { return (dir / "state" / "world_edits.lst").string(); }

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
		  + kOtherSha + " refs/heads/other\n"
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
	// Дату в этом случае пишет выкладка, прямо в ту же строку.
	EXPECT_TRUE(world_revision::Date().empty());
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
	EXPECT_TRUE(world_revision::Date().empty());
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

TEST_F(WorldRevision, TakesTheDateWhenTheTreeArrivedAtThisRevision) {
	// Последняя подряд идущая запись reflog, приводящая к нашему хешу: уход на
	// другую ревизию и возврат обратно -- это новая выкладка, а не прежняя.
	Write(".git/HEAD", "ref: refs/heads/master\n");
	Write(".git/refs/heads/master", kSha + "\n");
	Write(".git/logs/HEAD",
		  kZeroSha + " " + kSha + " bylins <b@m> 1000000000 +0100\tclone: from github\n"
		  + kSha + " " + kOtherSha + " bylins <b@m> 1700000000 +0100\tpull\n"
		  + kOtherSha + " " + kSha + " bylins <b@m> 1795685242 +0100\tpull\n"
		  + kSha + " " + kSha + " bylins <b@m> 1795685271 +0100\tcheckout: master to master\n");
	world_revision::Load(dir.string());
	EXPECT_EQ(LocalDate(1795685242), world_revision::Date());
}

TEST_F(WorldRevision, NoDateWhenTheReflogDoesNotEndOnThisRevision) {
	Write(".git/HEAD", "ref: refs/heads/master\n");
	Write(".git/refs/heads/master", kSha + "\n");
	Write(".git/logs/HEAD", kSha + " " + kOtherSha + " bylins <b@m> 1700000000 +0100\tpull\n");
	world_revision::Load(dir.string());
	EXPECT_EQ("b047e9a22 (master)", world_revision::Get());
	EXPECT_TRUE(world_revision::Date().empty());
}

TEST_F(WorldRevision, ListsZoneOncePerKindInVnumOrder) {
	// Вноситься зона может несколько раз (комнаты и мобы пишутся отдельно), а в
	// списке должна быть одной записью.
	world_revision::Load(dir.string(), StatePath());
	world_revision::NoteZoneSaved(90002, "мобы");
	world_revision::NoteZoneSaved(90001, "комнаты");
	world_revision::NoteZoneSaved(90001, "мобы");
	world_revision::NoteZoneSaved(90001, "мобы");
	const std::string edits = world_revision::FormatEdits();
	EXPECT_NE(std::string::npos, edits.find("90001 (комнаты+мобы, "));
	EXPECT_NE(std::string::npos, edits.find("90002 (мобы, "));
	EXPECT_LT(edits.find("90001"), edits.find("90002"));
}

TEST_F(WorldRevision, KeepsEditsAcrossARestartOnTheSameRevision) {
	Write(".git/HEAD", kSha + "\n");
	world_revision::Load(dir.string(), StatePath());
	world_revision::NoteZoneSaved(90101, "комнаты");
	// "Перезагрузка": тот же мир, тот же файл состояния.
	world_revision::Load(dir.string(), StatePath());
	EXPECT_NE(std::string::npos, world_revision::FormatEdits().find("90101 (комнаты, "));
}

TEST_F(WorldRevision, ForgetsEditsWhenAnotherRevisionIsDeployed) {
	Write(".git/HEAD", kSha + "\n");
	world_revision::Load(dir.string(), StatePath());
	world_revision::NoteZoneSaved(90102, "триггеры");
	ASSERT_FALSE(world_revision::FormatEdits().empty());
	// Выложили другой мир -- прежние расхождения уехали вместе с ним.
	Write(".git/HEAD", kOtherSha + "\n");
	world_revision::Load(dir.string(), StatePath());
	EXPECT_EQ("3e0febd1e", world_revision::Get());
	EXPECT_TRUE(world_revision::FormatEdits().empty());
}

TEST_F(WorldRevision, DoesNotWriteAnythingWithoutAStatePath) {
	Write(".git/HEAD", kSha + "\n");
	world_revision::Load(dir.string(), "");
	world_revision::NoteZoneSaved(90103, "зона");
	EXPECT_NE(std::string::npos, world_revision::FormatEdits().find("90103"));
	EXPECT_FALSE(fs::exists(dir / "state"));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
