/**
\file zone_exp.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"

#include <map>

namespace ZoneExpStat {

struct zone_data {
  zone_data() : gain(0), lose(0) {};

  unsigned long gain;
  unsigned long lose;
};

typedef std::map<int /* zone vnum */, zone_data> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long exp) {
	if (!exp) {
		return;
	}

	auto i = zone_list.find(zone_vnum);
	if (i != zone_list.end()) {
		if (exp > 0) {
			i->second.gain += exp;
		} else {
			i->second.lose += -exp;
		}
	} else {
		zone_data tmp_zone;
		if (exp > 0) {
			tmp_zone.gain = exp;
		} else {
			tmp_zone.lose = -exp;
		}
		zone_list[zone_vnum] = tmp_zone;
	}
}

void print_gain(CharData *ch) {
	if (!ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar(ch, "Пока в разработке.\r\n");
		return;
	}

	std::map<unsigned long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second.gain > 0) {
			tmp_list[i->second.gain] = i->first;
		}
	}
	if (!tmp_list.empty()) {
		SendMsgToChar(ch,
					  "Gain exp stats:\r\n"
					  "Total zones: %zu\r\n"
					  "  vnum - exp\r\n"
					  "================\r\n", tmp_list.size());
	} else {
		SendMsgToChar(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (std::map<unsigned long, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend;
		 ++i) {
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
	}
	page_string(ch->desc, out.str());
}

void print_log() {
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second.gain > 0 || i->second.lose > 0) {
			log("ZoneExp: %d %lu %lu", i->first, i->second.gain, i->second.lose);
		}
	}
}

} // ZoneExpStat

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
