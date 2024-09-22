/**
\file do_statistic.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "gameplay/statistics/mob_stat.h"

#include <map>

void PrintPair(std::ostringstream &out, int column_width, int val1, int val2);
void PrintValue(std::ostringstream &out, int column_width, int val);
void PrintUptime(std::ostringstream &out);

void do_statistic(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	static std::unordered_map<ECharClass, std::pair<int, int>> players;
	if (players.empty()) {
		for (const auto &char_class: MUD::Classes()) {
			if (char_class.IsAvailable()) {
				players.emplace(char_class.GetId(), std::pair<int, int>());
			}
		}
	} else {
		for (auto &it : players) {
			it.second.first = 0;
			it.second.second = 0;
		}
	}

	int clan{0}, noclan{0}, hilvl{0}, lowlvl{0}, rem{0}, norem{0}, pk{0}, nopk{0}, total{0};
	for (const auto &tch : character_list) {
		if (tch->IsNpc() || GetRealLevel(tch) >= kLvlImmortal || !HERE(tch)) {
			continue;
		}
		CLAN(tch) ? ++clan : ++noclan;
		GetRealRemort(tch) >= 1 ? ++rem : ++norem;
		pk_count(tch.get()) >= 1 ? ++pk : ++nopk;

		if (GetRealLevel(tch) >= 25) {
			++players[(tch->GetClass())].first;
			++hilvl;
		} else {
			++players[(tch->GetClass())].second;
			++lowlvl;
		}
		++total;
	}
	/*
	 * Тут нужно использовать форматирование таблицы
	 * \todo table format
	 */
	std::ostringstream out;
	out << kColorBoldCyn << " Статистика по персонажам в игре (всего / 25 ур. и выше / ниже 25 ур.):"
		<< kColorNrm << "\r\n" << "\r\n" << " ";
	int count{1};
	const int columns{2};
	const int class_name_col_width{15};
	const int number_col_width{3};
	for (const auto &it : players) {
		out << std::left << std::setw(class_name_col_width) << MUD::Class(it.first).GetPluralName() << " "
			<< kColorBoldRed << "[" << kColorBoldCyn
			<< std::setw(number_col_width) << std::right << it.second.first + it.second.second
			<< kColorBoldRed << "|" << kColorBoldCyn
			<< std::setw(number_col_width) << std::right << it.second.first
			<< kColorBoldRed << "|" << kColorBoldCyn
			<< std::setw(number_col_width) << std::right << it.second.second
			<< kColorBoldRed << "]" << kColorNrm;
		if (count % columns == 0) {
			out << "\r\n" << " ";
		} else {
			out << "  ";
		}
		++count;
	}
	out << "\r\n";

	const int headline_width{33};

	out << std::left << std::setw(headline_width) << " Всего игроков:";
	PrintValue(out, number_col_width, total);

	out << std::left << std::setw(headline_width) << " Игроков выше|ниже 25 уровня:";
	PrintPair(out, number_col_width, hilvl, lowlvl);

	out << std::left << std::setw(headline_width) << " Игроков с перевоплощениями|без:";
	PrintPair(out, number_col_width, rem, norem);

	out << std::left << std::setw(headline_width) << " Клановых|внеклановых игроков:";
	PrintPair(out, number_col_width, clan, noclan);

	out << std::left << std::setw(headline_width) << " Игроков с флагами ПК|без ПК:";
	PrintPair(out, number_col_width, pk, nopk);

	out << std::left << std::setw(headline_width) << " Героев (без ПК) | Тварей убито:";
	const int kills_col_width{5};
	PrintPair(out, kills_col_width, char_stat::players_killed, char_stat::mobs_killed);
	out << "\r\n";

	char_stat::PrintClassesExpStat(out);

	out << " Времени с перезагрузки: ";
	PrintUptime(out);

	SendMsgToChar(out.str(), ch);
}

void PrintPair(std::ostringstream &out, int column_width, int val1, int val2) {
	out << kColorBoldRed << "[" << kColorBoldCyn << std::right << std::setw(column_width) << val1
		<< kColorBoldRed << "|" << kColorBoldCyn << std::setw(column_width) << val2 << kColorBoldRed << "]" << kColorNrm << "\r\n";
}

void PrintValue(std::ostringstream &out, int column_width, int val) {
	out << kColorBoldRed << "[" << kColorBoldCyn << std::right << std::setw(column_width) << val << kColorBoldRed << "]" << kColorNrm << "\r\n";
}

void PrintUptime(std::ostringstream &out) {
	auto uptime = time(nullptr) - shutdown_parameters.get_boot_time();
	auto d = uptime / 86400;
	auto h = (uptime / 3600) % 24;
	auto m = (uptime / 60) % 60;
	auto s = uptime % 60;

	out << std::setprecision(2) << d << "д " << h << ":" << m << ":" << s << "\r\n";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
