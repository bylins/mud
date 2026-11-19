// Поиск команды идёт по префиксу и берёт ПЕРВОЕ совпадение в таблице (command_interpreter,
// strncmp по длине введённого). Значит короткое имя обязано стоять раньше любого, что с него
// начинается, иначе набрать его целиком невозможно: так "ban" уезжал в "bank", и бог получал
// "Это нельзя сделать здесь" вместо списка запретов.
//
// Тест стоит сторожем на весь список: добавил команду не на своё место -- узнаешь здесь, а не
// от бога, который год не может попасть в свою команду.

#include <gtest/gtest.h>

#include "engine/ui/interpreter.h"

#include <cstring>
#include <string>
#include <vector>

namespace interpreter_command_shadowing_test {

std::vector<std::string> AllCommands() {
	std::vector<std::string> names;
	for (int cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd) {
		names.emplace_back(cmd_info[cmd].command);
	}
	return names;
}

TEST(Interpreter_CommandTable, EveryCommandIsReachableByItsFullName) {
	const auto names = AllCommands();
	ASSERT_GT(names.size(), 100u) << "таблица команд не прочиталась";

	for (size_t i = 0; i < names.size(); ++i) {
		for (size_t j = 0; j < i; ++j) {
			// Запись j стоит раньше. Если она начинается с имени i, то ввод имени i целиком
			// совпадёт сперва с ней -- команда i недостижима по своему имени.
			if (names[j] != names[i] && names[j].compare(0, names[i].size(), names[i]) == 0) {
				FAIL() << "команда \"" << names[i] << "\" (позиция " << i
					   << ") затенена записью \"" << names[j] << "\" (позиция " << j
					   << "): более короткое имя должно стоять раньше";
			}
		}
	}
}

TEST(Interpreter_CommandTable, BanComesBeforeBank) {
	// Та самая пара, с которой всё началось: отдельным тестом, чтобы при перестановке
	// таблицы было видно, о чём речь, а не только "что-то затенено".
	const auto names = AllCommands();
	const auto ban = std::find(names.begin(), names.end(), "ban");
	const auto bank = std::find(names.begin(), names.end(), "bank");
	ASSERT_NE(ban, names.end());
	ASSERT_NE(bank, names.end());
	EXPECT_LT(ban - names.begin(), bank - names.begin());
}

}   // namespace interpreter_command_shadowing_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
