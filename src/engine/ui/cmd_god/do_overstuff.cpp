/**
\file do_page_clan_overstuff.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"

const int overstuff_threshold{8};

void DoPageClanOverstuff(CharData *ch, char *, int, int) {
	std::map<std::string, int> objects;
	for (const auto & clan : Clan::ClanList) {
		for (ObjData *chest = world[GetRoomRnum(clan->get_chest_room())]->contents; chest;
			 chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
					if (temp->get_auto_mort_req() > overstuff_threshold) {
						if (objects.count(clan->get_abbrev())) {
							objects[clan->get_abbrev()] += 1;
						} else {
							objects.insert(std::pair<std::string, int>(clan->get_abbrev(), 1));
						}
					}
				}
			}
		}
	}

	for (auto & object : objects) {
		sprintf(buf, "Дружина: %s, количество объектов: %d\r\n", object.first.c_str(), object.second);
		SendMsgToChar(buf, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
