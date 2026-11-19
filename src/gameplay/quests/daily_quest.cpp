#include "daily_quest.h"

#include "utils/parser_wrapper.h"   // issue.daily-quest: ParserWrapper вместо прямого pugixml

#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/global_objects.h"
#include "administration/accounts.h"
#include "engine/entities/zone.h"
#include "gameplay/economics/currencies.h"
#include "engine/core/comm.h"

#include <cstring>
#include <sstream>

namespace DailyQuest {

DailyQuest::DailyQuest(const std::string &desk, int reward)
	: desk(desk)
	, reward(reward)
{
}

namespace {
// Диагностика последней загрузки (для вывода богу по 'reload daily').
std::string g_last_load_message;
}

const std::string &GetLastLoadMessage() {
	return g_last_load_message;
}

// issue.daily-quest: data = корневой элемент <daily_quest> (CfgManager + ParserWrapper).
// Список собирается во временную карту и подменяет глобальный ТОЛЬКО при успехе - при
// любой ошибке разбора текущие дейлики остаются нетронутыми (как и в прежней версии).
void DailyQuestLoader::Load(parser_wrapper::DataNode data) {
	std::stringstream log_msg;
	DailyQuestMap tmp_list;


	for (auto &object : data.Children("quest")) {
		const char *raw_id = object.GetValue("id");
		const char *raw_reward = object.GetValue("reward");
		const char *raw_desk = object.GetValue("desk");
		const std::string attr_id = raw_id ? raw_id : "";
		const std::string attr_reward = raw_reward ? raw_reward : "";
		const std::string attr_desk = raw_desk ? raw_desk : "";

		if (attr_id.empty() || attr_reward.empty() || attr_desk.empty()) {
			log_msg << "Чтений файла дейликов прервано. Найден пустой элемент.";
			g_last_load_message = log_msg.str();
			mudlog(g_last_load_message.c_str(), CMP, kLvlImmortal, SYSLOG, true);
			return;
		}

		int id;
		int reward;
		try {
			id = std::stoi(attr_id);
			reward = std::stoi(attr_reward);
		} catch (const std::invalid_argument &ia) {
			log_msg << "Чтений файла дейликов прервано: найдено некорректное число";
			g_last_load_message = log_msg.str();
			mudlog(g_last_load_message.c_str(), CMP, kLvlImmortal, SYSLOG, true);
			return;
		} catch (const std::out_of_range &oor) {
			log_msg << "Чтений файла дейликов прервано: слишком большое число";
			g_last_load_message = log_msg.str();
			mudlog(g_last_load_message.c_str(), CMP, kLvlImmortal, SYSLOG, true);
			return;
		}

		tmp_list.try_emplace(id, attr_desk, reward);
	}

	GlobalObjects::daily_quests() = std::move(tmp_list);
	log_msg << "Daily quests file loading successful. Total quests: " << GlobalObjects::daily_quests().size();
	g_last_load_message = log_msg.str();
	mudlog(g_last_load_message.c_str(), CMP, kLvlImmortal, SYSLOG, true);
}

void DailyQuestLoader::Reload(parser_wrapper::DataNode data) {
	Load(std::move(data));
}


void DoQuest(CharData *ch, int id) {
	const auto quest = MUD::daily_quests().find(id);
	if (quest == MUD::daily_quests().end()) {
		log("Quest Id: %d - не найден", id);
		return;
	}
	const auto account = ch->get_account();
	if (!account) {
		return;
	}
	if (!account->quest_is_available(id)) {
		SendMsgToChar(ch, "Сегодня вы уже получали гривны за выполнение этого задания.\r\n");
		return;
	}
	// Награда идёт полной суммой. Прежде её резали дважды: обнуляли совсем, если зона
	// низкоуровневая (средний уровень мобов <= 12) и на аккаунте есть чар с
	// level + remorts/5 >= 20, и делили пополам, если уровень зоны не выше уровня игрока. Обе
	// отсечки били не по тем: у нас игроки возвращаются и начинают с первого уровня, и аккаунт
	// со старым чаром ронял их награду в ноль, а деление пополам доставалось ровно тем
	// аккаунтам, где прокачанного чара нет, то есть настоящим новичкам. Защищали они при этом
	// гроши: самый дешёвый дейлик (болото в 43-й) даёт 2--4 гривны при капе 1500 на руках, и
	// суточный лимит на задание держится отдельно -- quest_is_available выше.
	const int value = quest->second.reward + number(1, 3);
	// Гривны идут всем, сколько бы перевоплощений ни было. Прежде до шестого морта вместо них
	// приходило "Глянув на непонятный слиток, вы решили выкинуть его..." -- то есть дейлики по
	// низким зонам не доходили ровно до тех, чей это контент, и до вернувшихся игроков, которые
	// начинают с первого уровня.
	// Задание считается полученным только если гривны и вправду начислены: иначе награду
	// отрезали (тестовая зона), а задание всё равно списывалось на сутки, и на следующем заходе
	// игрок читал "сегодня вы уже получали гривны за выполнение этого задания", не получив ни
	// одной. Предмет с триггера выдаёт сам триггер и от этого счётчика не зависит, так что не
	// списывать здесь нечем навредить.
	if (zone_table[world[ch->in_room]->zone_rn].under_construction) {
		SendMsgToChar(ch, "Зона тестовая, вашу гривну отобрали боги.\r\n");
		return;
	}
	// валюта дейлика жёстко задана до переписывания системы квестов
	const long added = currencies::AddHand(*ch, currencies::kCopperGrivnaId, value, true);
	if (added <= 0) {
		// Начислить не вышло (кап валюты на руках, отключённая валюта). Начислять нечего --
		// и списывать задание тоже.
		return;
	}
	log("Персонаж %s получил %ld единиц награды дневного задания.", GET_NAME(ch), added);
	account->complete_quest(id);
}

bool OnCurrencySpent(CharData *ch, const std::string &currency, long amount) {
	// трата валюты дейлика (kCopperGrivna) учитывается; превышение порога сбрасывает дейлики
	if (currency != currencies::kCopperGrivnaId) {
		return false;
	}
	ch->spent_hryvn_sub(static_cast<int>(amount));
	if (ch->get_spent_hryvn() > 1000) {
		ch->reset_daily_quest();
		return true;
	}
	return false;
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
