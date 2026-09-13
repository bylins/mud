/**
\file do_check_zone_occupation.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"

void CheckCharactersInZone(ZoneRnum zone_nr, CharData *ch) {
	DescriptorData *i;
	int rnum_start, rnum_stop;
	bool found = false;

	if (room_spells::IsZoneRoomAffected(zone_nr, room_spells::ERoomAffect::kRuneLabel)) {
		SendMsgToChar("В зоне имеется рунная метка.\r\n", ch);
	}

	for (i = descriptor_list; i; i = i->next) {
		if  (i->state != EConState::kPlaying)
			continue;
		if (i->character->in_room == kNowhere)
			continue;
		if (GetRealLevel(i->character) >= kLvlImmortal)
			continue;
		if (world[i->character->in_room]->zone_rn != zone_nr)
			continue;
		SendMsgToChar(fmt::format("Проверка по дискрипторам: В зоне (vnum: {} клетка: {}) находится персонаж: {}.\r\n",
								  zone_table[zone_nr].vnum, GET_ROOM_VNUM(i->character->in_room),
								  GET_NAME(i->character)), ch);
		found = true;
	}
	if (found)
		return;
	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!GetZoneRooms(zone_nr, &rnum_start, &rnum_stop)) {
		SendMsgToChar(fmt::format("Нет комнат в зоне {}.", static_cast<int>(zone_table[zone_nr].vnum)), ch);
		return;    // в зоне нет комнат :)
	}

	for (; rnum_start <= rnum_stop; rnum_start++) {
		// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		{
			for (const auto c : world[rnum_start]->people) {
				if (!c->IsNpc() && (GetRealLevel(c) < kLvlImmortal)) {
					SendMsgToChar(fmt::format("Проверка по списку чаров (с учетом linkdrop): в зоне vnum: {} клетка: {} находится персонаж: {}.\r\n",
											  zone_table[zone_nr].vnum, GET_ROOM_VNUM(c->in_room), GET_NAME(c)), ch);
					found = true;
				}
			}
		}
	}

	// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (const auto c : world[kStrangeRoom]->people) {
		int was = c->get_was_in_room();
		if (was == kNowhere
			|| GetRealLevel(c) >= kLvlImmortal
			|| world[was]->zone_rn != zone_nr) {
			continue;
		}

		SendMsgToChar(fmt::format("В прокси руме сидит игрок {} находящийся в зоне vnum: {} клетка: {}\r\n",
								  GET_NAME(c), zone_table[zone_nr].vnum, GET_ROOM_VNUM(c->in_room)), ch);
		found = true;
	}

	if (!found) {
		SendMsgToChar(fmt::format("В зоне {} даже мышь не пробегала.\r\n", zone_table[zone_nr].vnum), ch);
	}
}

void DoCheckZoneOccupation(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int number;
	ZoneRnum zrn;
	char vnum_arg[kMaxInputLength];
	one_argument(argument, vnum_arg);
	bool is_found = false;
	if (!*vnum_arg || !a_isdigit(*vnum_arg)) {
		SendMsgToChar("Usage: занятость внумзоны\r\n", ch);
		return;
	}

	if ((number = atoi(vnum_arg)) < 0) {
		SendMsgToChar("Такого внума не может быть!\r\n", ch);
		return;
	}

	// что-то по другому не нашел, как проверить существует такая зона или нет
	for (zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
		if (zone_table[zrn].vnum == number) {
			CheckCharactersInZone(zrn, ch);
			is_found = true;
			break;
		}
	}

	if (!is_found) {
		SendMsgToChar("Такой зоны нет.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
