#pragma once

#include <string>
#include <unordered_map>

class CharData;

namespace DailyQuest {

struct DailyQuest {
	std::string desk;
	int reward;

	DailyQuest(const std::string &desk, int reward);
};

using DailyQuestMap = std::unordered_map<int, DailyQuest>;

// загрузка файла с дейликами
// если ch будет валиден - то он получит сообщение с статусом загрузки файла
void LoadFromFile(CharData *ch = nullptr);

// Выдать награду за дейлик (логика вынесена из Player::dquest).
void DoQuest(CharData *ch, int id);

// Учёт траты валюты при покупке (валюта дейлика = kCopperGrivna); true, если дейлики сброшены.
bool OnCurrencySpent(CharData *ch, const std::string &currency, long amount);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
