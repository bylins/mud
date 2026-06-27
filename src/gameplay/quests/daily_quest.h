#pragma once

#include "engine/boot/cfg_manager.h"   // issue.daily-quest: загрузка через CfgManager

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

// issue.daily-quest: дейлики грузятся из cfg/quests/daily_quest.xml через CfgManager
// (ParserWrapper). Формат не менялся. При ошибке разбора текущий список квестов
// сохраняется (подмена только при успешной загрузке).
class DailyQuestLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// Диагностика последней загрузки (для вывода богу по 'reload daily').
const std::string &GetLastLoadMessage();

// Выдать награду за дейлик (логика вынесена из Player::dquest).
void DoQuest(CharData *ch, int id);

// Учёт траты валюты при покупке (валюта дейлика = kCopperGrivna); true, если дейлики сброшены.
bool OnCurrencySpent(CharData *ch, const std::string &currency, long amount);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
