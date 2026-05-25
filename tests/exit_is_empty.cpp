// Part of Bylins http://www.mud.ru
// Regression tests for ExitData::is_empty (issue #3272).
//
// Background: пустые D-блоки в .wld/yaml/sqlite (всё нулевое: to_room=0,
// нет keyword/vkeyword/description, exit_info=0, key<=0, lock_complexity=0)
// раньше доходили до рантайма и палились в `show errors`. Все три загрузчика
// теперь дропают такие блоки на этапе загрузки; эта функция фиксирует, что
// считать "полностью пустым", чтобы фильтр случайно не съел осмысленный выход.

#include "engine/entities/room_data.h"
#include "engine/structs/structs.h"

#include <gtest/gtest.h>

TEST(ExitDataIsEmpty, DefaultConstructedIsEmpty) {
	ExitData e;
	EXPECT_TRUE(e.is_empty());
}

TEST(ExitDataIsEmpty, ToRoomSetMakesNonEmpty) {
	ExitData e;
	e.to_room(42);
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, ExitInfoFlagMakesNonEmpty) {
	// Случай вроде [15000] D0: 8 0 0 0 -- декоративная скрытая дверь без to_room.
	// Фильтр не должен её схлопывать.
	ExitData e;
	e.exit_info = 8;
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, DescriptionMakesNonEmpty) {
	ExitData e;
	e.general_description = "look-only door description";
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, KeywordMakesNonEmpty) {
	ExitData e;
	e.set_keyword("дверь");
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, VkeywordMakesNonEmpty) {
	ExitData e;
	e.set_vkeyword("дверь");
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, KeyMakesNonEmpty) {
	ExitData e;
	e.key = 1234;
	EXPECT_FALSE(e.is_empty());
}

TEST(ExitDataIsEmpty, NegativeKeyIsStillEmpty) {
	// key = -1 это "нет ключа", такой блок считается мусором.
	ExitData e;
	e.key = -1;
	EXPECT_TRUE(e.is_empty());
}

TEST(ExitDataIsEmpty, LockComplexityMakesNonEmpty) {
	ExitData e;
	e.lock_complexity = 5;
	EXPECT_FALSE(e.is_empty());
}
