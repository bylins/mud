// Граница кодировки у файлов состояния (state/) -- issue #3787.
//
// state/ хранится в нативной кодировке движка: записи уходят на диск как есть, а чтение
// принимает и старый KOI8-R, и уже переведённый UTF-8, разбирая каждую строку отдельно
// (native_text::from_disk_line). Тесты держат оба конца этой границы:
//   * запись не переводит текст -- иначе первое же сохранение вернуло бы файл в KOI8-R,
//     ровно так и жил cfg до #3794;
//   * цикл load/save байт в байт воспроизводит файл -- защита от повторного перевода,
//     который однажды раздул obj_sets.xml до 6.7 ГБ (см. parser_wrapper.encoding.cpp);
//   * файл, оставшийся в KOI8-R, читается правильно, и переведённый наполовину -- тоже.
//
// Сам тест -- чистый ASCII: кириллица записана байтовыми escape-последовательностями UTF-8
// и приводится к нативной кодировке через native_text.

#include "engine/boot/state_manager.h"
#include "utils/native_text.h"
#include "utils/translit_koi8.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

// "Стрибог" и "Мышь" в UTF-8: имена персонажей -- ровно то, что лежит в этих списках.
const char *const kStateNameUtf8 = "\xD0\xA1\xD1\x82\xD1\x80\xD0\xB8\xD0\xB1\xD0\xBE\xD0\xB3";
const char *const kStateOtherUtf8 = "\xD0\x9C\xD1\x8B\xD1\x88\xD1\x8C";

std::string NativeFromUtf8(const char *utf8) {
	return native_text::from_koi8(codepages::Utf8ToKoi8(utf8));
}

std::string ReadStateFileBytes(const std::filesystem::path &path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

// StateManager отдаёт пути относительно каталога мира, так что тест работает в своём
// временном каталоге и возвращает прежний, что бы ни случилось с проверками.
class StateTestDir {
 public:
	StateTestDir() : previous_(std::filesystem::current_path()),
					 root_(std::filesystem::temp_directory_path() / "bylins_state_test") {
		std::filesystem::remove_all(root_);
		std::filesystem::create_directories(root_ / "state");
		std::filesystem::current_path(root_);
	}
	~StateTestDir() {
		std::filesystem::current_path(previous_);
		std::error_code ec;
		std::filesystem::remove_all(root_, ec);
	}

 private:
	std::filesystem::path previous_;
	std::filesystem::path root_;
};

}  // namespace

TEST(StateManagerEncoding, SaveLinesWritesNativeBytes) {
	StateTestDir dir;
	const state::StateManager manager;
	const std::string name = NativeFromUtf8(kStateNameUtf8);

	ASSERT_TRUE(manager.SaveLines(state::EStateFile::kApprovedNames, {name}));

	// На диске должно лежать ровно то, что было в памяти: границы записи здесь больше нет.
	EXPECT_EQ(ReadStateFileBytes(manager.Path(state::EStateFile::kApprovedNames)), name + "\n");
}

TEST(StateManagerEncoding, SaveLoadSaveIsByteIdentical) {
	StateTestDir dir;
	const state::StateManager manager;
	const std::vector<std::string> lines{NativeFromUtf8(kStateNameUtf8), NativeFromUtf8(kStateOtherUtf8)};

	ASSERT_TRUE(manager.SaveLines(state::EStateFile::kApprovedNames, lines));
	const std::string first = ReadStateFileBytes(manager.Path(state::EStateFile::kApprovedNames));

	const auto loaded = manager.LoadLines(state::EStateFile::kApprovedNames);
	EXPECT_EQ(loaded, lines) << "строки должны вернуться ровно такими, какими ушли";

	ASSERT_TRUE(manager.SaveLines(state::EStateFile::kApprovedNames, loaded));
	EXPECT_EQ(ReadStateFileBytes(manager.Path(state::EStateFile::kApprovedNames)), first)
			<< "цикл load/save обязан воспроизводить файл байт в байт";
}

TEST(StateManagerEncoding, ReadsAListLeftInKoi8) {
	StateTestDir dir;
	const state::StateManager manager;
	{
		std::ofstream out(manager.Path(state::EStateFile::kApprovedNames), std::ios::binary);
		out << codepages::Utf8ToKoi8(kStateNameUtf8) << "\n";
	}

	const auto loaded = manager.LoadLines(state::EStateFile::kApprovedNames);
	ASSERT_EQ(loaded.size(), 1u);
	EXPECT_EQ(loaded[0], NativeFromUtf8(kStateNameUtf8));
}

TEST(StateManagerEncoding, ReadsAHalfConvertedList) {
	// Так выглядит список, который движок уже дописывал нативным, пока файл лежал в KOI8-R:
	// решение о кодировке принимается для каждой строки отдельно, поэтому читается и такой.
	StateTestDir dir;
	const state::StateManager manager;
	{
		std::ofstream out(manager.Path(state::EStateFile::kApprovedNames), std::ios::binary);
		out << codepages::Utf8ToKoi8(kStateNameUtf8) << "\n" << NativeFromUtf8(kStateOtherUtf8) << "\n";
	}

	const auto loaded = manager.LoadLines(state::EStateFile::kApprovedNames);
	ASSERT_EQ(loaded.size(), 2u);
	EXPECT_EQ(loaded[0], NativeFromUtf8(kStateNameUtf8));
	EXPECT_EQ(loaded[1], NativeFromUtf8(kStateOtherUtf8));
}

TEST(StateManagerEncoding, SaveTextAndLoadTextMirrorEachOther) {
	StateTestDir dir;
	const state::StateManager manager;
	const std::string body = "<mobs>" + NativeFromUtf8(kStateNameUtf8) + "</mobs>\n";

	ASSERT_TRUE(manager.SaveText(state::EStateFile::kUniqueMobs, body));
	EXPECT_EQ(ReadStateFileBytes(manager.Path(state::EStateFile::kUniqueMobs)), body);
	EXPECT_EQ(manager.LoadText(state::EStateFile::kUniqueMobs), body);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
