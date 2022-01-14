#pragma once

#include <string>
#include <unordered_map>

class CHAR_DATA;

namespace DailyQuest {

struct DailyQuest {
	// описание
	std::string desk;
	// награда
	int reward;

	DailyQuest(const std::string &desk, int reward);
};

using DailyQuestMap = std::unordered_map<int, DailyQuest>;

// загрузка файла с дейликами
// если ch будет валиден - то он получит сообщение с статусом загрузки файла
void load_from_file(CHAR_DATA *ch = nullptr);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
