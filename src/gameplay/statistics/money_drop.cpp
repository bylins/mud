/**
\file money_drop.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"

#include <map>

namespace MoneyDropStat {

typedef std::map<int /* zone vnum */, long /* money count */> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long money) {
	auto i = zone_list.find(zone_vnum);
	if (i != zone_list.end()) {
		i->second += money;
	} else {
		zone_list[zone_vnum] = money;
	}
}

void print(CharData *ch) {
	if (!IS_GRGOD(ch)) {
		SendMsgToChar(ch, "Только для иммов 33+.\r\n");
		return;
	}

	std::map<long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second > 0) {
			tmp_list[i->second] = i->first;
		}
	}
	if (!tmp_list.empty()) {
		SendMsgToChar(ch,
					  "Money drop stats:\r\n"
					  "Total zones: %zu\r\n"
					  "  vnum - money\r\n"
					  "================\r\n", tmp_list.size());
	} else {
		SendMsgToChar(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (auto i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i) {
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
//		SendMsgToChar(ch, "  %4d - %ld\r\n", i->second, i->first);
	}
	page_string(ch->desc, out.str());
}

void print_log() {
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second > 0) {
			log("ZoneDrop: %d %ld", i->first, i->second);
		}
	}
}

} // MoneyDropStat

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
