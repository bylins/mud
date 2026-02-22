/**
\file do_show_zone_stat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "engine/ui/modify.h"

extern time_t zones_stat_date;

bool comp(std::pair <int, int> a, std::pair<int, int> b) {
	return a.second > b.second;
}

void PrintZoneStat(CharData *ch, int start, int end, bool sort) {
	std::stringstream ss;

	if (end == 0)
		end = start;
	std::vector<std::pair<int, int>> zone;
	for (ZoneRnum i = start; i < static_cast<ZoneRnum>(zone_table.size()) && i <= end; i++) {
		zone.emplace_back(i, zone_table[i].traffic);
	}
	if (sort) {
		std::sort(zone.begin(), zone.end(), comp);
	}
	for (auto it : zone) {
		ss << "Zone: " << zone_table[it.first].vnum << " count_reset с ребута: " << zone_table[it.first].count_reset
		   << ", посещено: " << zone_table[it.first].traffic << ", название зоны: " << zone_table[it.first].name<< "\r\n";
	}
	page_string(ch->desc, ss.str());
}

void DoShowZoneStat(CharData *ch, char *argument, int, int) {
	std::string buffer;
	char arg1[kMaxInputLength], arg2[kMaxInputLength], arg3[kMaxInputLength];
	bool sort = false;

	three_arguments(argument, arg1, arg2, arg3);
	if (!str_cmp(arg2, "-s") || !str_cmp(arg3, "-s"))
		sort = true;
	if (!*arg1) {
		SendMsgToChar(ch, "Зоныстат формат: 'все' или диапазон через пробел, -s в конце для сортировки. 'очистить' новая таблица\r\n");
		return;
	}
	if (!str_cmp(arg1, "очистить")) {
		const time_t ct = time(nullptr);
		char *date = asctime(localtime(&ct));
		SendMsgToChar(ch, "Начинаю новую запись статистики от %s", date);
		zones_stat_date = ct;
		for (auto & i : zone_table) {
			i.traffic = 0;
		}
		ZoneTrafficSave();
		return;
	}
	SendMsgToChar(ch, "Статистика с %s", asctime(localtime(&zones_stat_date)));
	if (!str_cmp(arg1, "все")) {
		PrintZoneStat(ch, 0, 999999, sort);
		return;
	}
	int tmp1 = GetZoneRnum(atoi(arg1));
	int tmp2 = GetZoneRnum(atoi(arg2));
	if (tmp1 != kNoZone && !*arg2) {
		PrintZoneStat(ch, tmp1, tmp1, sort);
		return;
	}
	if (tmp1 != kNoZone && tmp2 != kNoZone) {
		PrintZoneStat(ch, tmp1, tmp2, sort);
		return;
	}
	SendMsgToChar(ch, "Зоныстат формат: 'все' или диапазон через пробел, -s в конце для сортировки. 'очистить' новая таблица\r\n");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
