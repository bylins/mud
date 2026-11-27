// Раскладка файлов legacy-мира -- issue #3301.
//
// LegacyFiles -- единственное место, которое знает, где под корнем лежат файлы каждого
// типа. Тесты держат саму раскладку (пути по типам, справка без подкаталогов) и файловые
// операции, которыми пользуются OLC-сохранение и ресейв: замену файла зоны, перестройку
// и дополнение index. Корень -- явный путь, текущий каталог процесса не меняется.

#include "engine/boot/legacy_files.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

namespace fs = std::filesystem;

std::string ReadAll(const fs::path &path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

void WriteAll(const fs::path &path, const std::string &text) {
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	out << text;
}

// Корень с цифрами в пути: номер зоны берётся только из имени файла, не из каталога.
class LegacyFilesTest : public ::testing::Test {
 protected:
	void SetUp() override {
		m_cwd = fs::current_path();
		m_root = fs::temp_directory_path() / "bylins_legacy_files_test" / "mud4000" / "world";
		fs::remove_all(m_root.parent_path().parent_path());
	}

	void TearDown() override {
		EXPECT_EQ(fs::current_path(), m_cwd);
		fs::remove_all(m_root.parent_path().parent_path());
	}

	fs::path m_cwd;
	fs::path m_root;
};

TEST_F(LegacyFilesTest, WorldLayout) {
	const auto files = LegacyFiles::World(m_root);
	EXPECT_EQ(files.Index(DB_BOOT_WLD), m_root / "wld" / "index");
	EXPECT_EQ(files.Index(DB_BOOT_MOB), m_root / "mob" / "index");
	EXPECT_EQ(files.Index(DB_BOOT_OBJ), m_root / "obj" / "index");
	EXPECT_EQ(files.Index(DB_BOOT_ZON), m_root / "zon" / "index");
	EXPECT_EQ(files.Index(DB_BOOT_TRG), m_root / "trg" / "index");
	EXPECT_EQ(files.Entry(DB_BOOT_MOB, "12.mob"), m_root / "mob" / "12.mob");
	EXPECT_EQ(files.ZoneFile(DB_BOOT_ZON, 123), m_root / "zon" / "123.zon");
	EXPECT_EQ(files.TempFile(DB_BOOT_TRG, 123), m_root / "trg" / "123.new");
	// Справка -- не часть мира.
	EXPECT_TRUE(files.Index(DB_BOOT_HLP).empty());
}

TEST_F(LegacyFilesTest, HelpLayoutIsFlat) {
	const auto files = LegacyFiles::Help(m_root);
	EXPECT_EQ(files.Index(DB_BOOT_HLP), m_root / "index");
	EXPECT_EQ(files.Entry(DB_BOOT_HLP, "spells.hlp"), m_root / "spells.hlp");
	EXPECT_TRUE(files.Index(DB_BOOT_WLD).empty());
	EXPECT_TRUE(files.ZoneFile(DB_BOOT_HLP, 1).empty());
	EXPECT_TRUE(files.TempFile(DB_BOOT_HLP, 1).empty());
}

TEST_F(LegacyFilesTest, EnsureLayoutCreatesSubdirs) {
	const auto files = LegacyFiles::World(m_root);
	files.EnsureLayout();
	for (const char *sub : {"wld", "mob", "obj", "zon", "trg"}) {
		EXPECT_TRUE(fs::is_directory(m_root / sub)) << sub;
	}
}

TEST_F(LegacyFilesTest, CommitZoneFileReplacesOld) {
	const auto files = LegacyFiles::World(m_root);
	files.EnsureLayout();
	WriteAll(files.ZoneFile(DB_BOOT_WLD, 40), "old\n");
	WriteAll(files.TempFile(DB_BOOT_WLD, 40), "new\n");
	ASSERT_TRUE(files.CommitZoneFile(DB_BOOT_WLD, 40));
	EXPECT_EQ(ReadAll(files.ZoneFile(DB_BOOT_WLD, 40)), "new\n");
	EXPECT_FALSE(fs::exists(files.TempFile(DB_BOOT_WLD, 40)));
	// Нечего ставить на место -- ошибка, а не тихий успех.
	EXPECT_FALSE(files.CommitZoneFile(DB_BOOT_WLD, 41));
}

TEST_F(LegacyFilesTest, RebuildIndexSortsNumerically) {
	const auto files = LegacyFiles::World(m_root);
	files.EnsureLayout();
	for (const char *name : {"100.obj", "20.obj", "3.obj", "20.new", "readme.txt"}) {
		WriteAll(m_root / "obj" / name, "$\n");
	}
	files.RebuildIndex(DB_BOOT_OBJ);
	EXPECT_EQ(ReadAll(files.Index(DB_BOOT_OBJ)), "3.obj\n20.obj\n100.obj\n$\n");
}

TEST_F(LegacyFilesTest, AddToIndexKeepsOrder) {
	const auto files = LegacyFiles::World(m_root);
	files.EnsureLayout();
	WriteAll(files.Index(DB_BOOT_TRG), "1.trg\n30.trg\n$\n");

	ASSERT_TRUE(files.AddToIndex(DB_BOOT_TRG, 12));
	EXPECT_EQ(ReadAll(files.Index(DB_BOOT_TRG)), "1.trg\n12.trg\n30.trg\n$\n");

	// Уже есть -- индекс не меняется.
	ASSERT_TRUE(files.AddToIndex(DB_BOOT_TRG, 30));
	EXPECT_EQ(ReadAll(files.Index(DB_BOOT_TRG)), "1.trg\n12.trg\n30.trg\n$\n");

	// Больше всех -- перед '$'.
	ASSERT_TRUE(files.AddToIndex(DB_BOOT_TRG, 4000));
	EXPECT_EQ(ReadAll(files.Index(DB_BOOT_TRG)), "1.trg\n12.trg\n30.trg\n4000.trg\n$\n");
}

TEST_F(LegacyFilesTest, AddToIndexWithoutIndexFails) {
	const auto files = LegacyFiles::World(m_root);
	files.EnsureLayout();
	EXPECT_FALSE(files.AddToIndex(DB_BOOT_TRG, 12));
}

} // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
