#include "engine/entities/char_data.h"

#include <gtest/gtest.h>

// Тест на баг: после копирования CharData через operator=,
// current_morph_->ch_ указывает на старый (возможно удалённый) объект.
// get_wis() вызывает current_morph_->GetWis() -> ch_->GetInbornWis(),
// что приводит к segfault если оригинал удалён.

TEST(CHAR_MorphCopy, AssignmentOperator_MorphPointerValid)
{
	// Создаём оригинальный персонаж
	CharData original;
	original.player_specials = std::make_shared<player_special_data>();
	original.set_wis(15);

	// Создаём новый персонаж и копируем в него через operator=
	CharData copy;
	copy.player_specials = std::make_shared<player_special_data>();
	copy = original;

	// После копирования get_wis() должен работать корректно
	// и возвращать скопированное значение
	EXPECT_EQ(15, copy.get_wis());
}

TEST(CHAR_MorphCopy, AssignmentOperator_OriginalDeleted_CopyStillWorks)
{
	CharData copy;
	copy.player_specials = std::make_shared<player_special_data>();

	{
		// Создаём оригинал в отдельном скоупе
		CharData original;
		original.player_specials = std::make_shared<player_special_data>();
		original.set_wis(20);

		// Копируем
		copy = original;

		// original будет уничтожен при выходе из скоупа
	}

	// После уничтожения оригинала copy.get_wis() НЕ должен падать
	// Это главный тест на баг - current_morph_->ch_ должен указывать на copy, а не на original
	EXPECT_EQ(20, copy.get_wis());
}

TEST(CHAR_MorphCopy, ArrayReallocation_SimulatesMobProtoResize)
{
	// Симуляция того, что происходит в medit.cpp при добавлении нового моба:
	// 1. Создаётся новый массив
	// 2. Копируются элементы из старого
	// 3. Старый массив удаляется
	// 4. Обращение к элементам нового массива

	const int OLD_SIZE = 3;
	const int NEW_SIZE = 4;

	// "Старый" массив (симуляция mob_proto)
	CharData* old_array = new CharData[OLD_SIZE];
	for (int i = 0; i < OLD_SIZE; ++i) {
		old_array[i].player_specials = std::make_shared<player_special_data>();
		old_array[i].set_wis(10 + i);
	}

	// "Новый" массив
	CharData* new_array = new CharData[NEW_SIZE];
	for (int i = 0; i < NEW_SIZE; ++i) {
		new_array[i].player_specials = std::make_shared<player_special_data>();
	}

	// Копируем элементы (как в medit.cpp строки 413, 418)
	for (int i = 0; i < OLD_SIZE; ++i) {
		new_array[i] = old_array[i];
	}

	// Удаляем старый массив (как в medit.cpp строка 437)
	delete[] old_array;
	old_array = nullptr;

	// Теперь обращаемся к get_wis() на элементах нового массива
	// Это должно работать, а не падать с segfault
	for (int i = 0; i < OLD_SIZE; ++i) {
		EXPECT_EQ(10 + i, new_array[i].get_wis());
	}

	delete[] new_array;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
