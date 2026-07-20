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

// Запись могла появиться из-за сброса поля объектов (истекла рента), и тогда в player лежит
// ноль, которого туда никто не писал. Это "снимка нет", а не "ожидаем нулевую сумму": игрок не
// должен при каждом входе поднимать имму ложную тревогу.
TEST(FileCrcBuffer, ZeroPlayerCrcIsTreatedAsMissingSnapshot) {
	const long uid = 7700010;
	const std::string objs = "object data";
	const std::string pfile = "player file";

	// рента истекла -> запись заведена со всеми нулями, player CRC в нее не писали
	FileCRC::update_from_content(uid, FileCRC::kTextObjs, objs.data(), objs.size());
	FileCRC::reset(uid, FileCRC::kTextObjs);

	// вход игрока: снимка player нет, значит его надо просто запомнить, а не ругаться
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kPlayer, pfile.data(), pfile.size()));
	// и со второго раза он уже работает как обычный снимок
	EXPECT_TRUE(FileCRC::verify_from_content(uid, FileCRC::kPlayer, pfile.data(), pfile.size()));
	const std::string other = "tampered file";
	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::kPlayer, other.data(), other.size()));
}

// А вот у файлов объектов ноль осмыслен: сервер удалил файл, и появление его заново -- повод
// для тревоги. Эта проверка не должна ослабнуть заодно с player.
TEST(FileCrcBuffer, ZeroObjectCrcStillGuards) {
	const long uid = 7700011;
	const std::string objs = "object data";

	FileCRC::update_from_content(uid, FileCRC::kTimeObjs, objs.data(), objs.size());
	FileCRC::reset(uid, FileCRC::kTimeObjs);

	EXPECT_FALSE(FileCRC::verify_from_content(uid, FileCRC::kTimeObjs, objs.data(), objs.size()));
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
