//
// Created by Sventovit on 08.09.2024.
//

#include "engine/entities/char_data.h"
#include "engine/db/world_objects.h"
#include "gameplay/economics/exchange.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/clans/house.h"
#include "gameplay/communication/parcel.h"

#include <third_party_libs/fmt/include/fmt/format.h>

void perform_immort_where(CharData *ch, char *arg);
void perform_mortal_where(CharData *ch, char *arg);
bool print_imm_where_obj(CharData *ch, char *arg, int num);
bool print_object_location(int num, const ObjData *obj, CharData *ch);

void do_where(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);

	if (IS_GRGOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}

void perform_immort_where(CharData *ch, char *arg) {
	DescriptorData *d;
	int num = 1, found = 0;
	std::stringstream ss;
	if (!*arg) {
		ss << "ИГРОКИ\r\n------\r\n";
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) == CON_PLAYING) {
				const auto i = d->get_character();
				if (i && CAN_SEE(ch, i) && (i->in_room != kNowhere)) {
					if (d->original) {
						ss << fmt::format("{:<20} - [{:>7}] {} (in {})\r\n",
										  GET_NAME(i),
										  GET_ROOM_VNUM(d->character->in_room),
										  world[d->character->in_room]->name,
										  GET_NAME(d->character));
					} else {
						ss << fmt::format("{:<20} - [{:>7}] {}\r\n", GET_NAME(i),
										  GET_ROOM_VNUM(i->in_room), world[i->in_room]->name);
					}
				}
			}
		}
		SendMsgToChar(ss.str(), ch);
	} else {
		for (const auto &i : character_list) {
			if (CAN_SEE(ch, i)
				&& i->in_room != kNowhere
				&& isname(arg, i->GetCharAliases())) {
				ZoneData *zone = &zone_table[world[i->in_room]->zone_rn];
				found = 1;
				ss << fmt::format("{:>3}. {} ({:>6}) {:<25} - [{:>7}] {}. Название зоны: '{}'\r\n",
								  num++,
								  i->IsNpc() ? "Моб:   " : "Игрок: ",
								  GET_MOB_VNUM(i),
								  GET_NAME(i),
								  GET_ROOM_VNUM(i->in_room),
								  world[i->in_room]->name,
								  zone->name.c_str());
			}
		}
		SendMsgToChar(ss.str(), ch);
		if (!print_imm_where_obj(ch, arg, num) && !found) {
			SendMsgToChar("Нет ничего похожего.\r\n", ch);
		}
	}
}

/**
* Иммский поиск шмоток по 'где' с проходом как по глобальному списку, так
* и по спискам хранилищ и почты.
*/
bool print_imm_where_obj(CharData *ch, char *arg, int num) {
	bool found = false;

	/* maybe it is possible to create some index instead of linear search */
	world_objects.foreach([&](const ObjData::shared_ptr& object) {
	  if (isname(arg, object->get_aliases())) {
		  if (print_object_location(num, object.get(), ch)) {
			  found = true;
			  num++;
		  }
	  }
	});

	int tmp_num = num;
	if (IS_GOD(ch)
		|| ch->IsFlagged(EPrf::kCoderinfo)) {
		tmp_num = Clan::print_imm_where_obj(ch, arg, tmp_num);
		tmp_num = Depot::print_imm_where_obj(ch, arg, tmp_num);
		tmp_num = Parcel::print_imm_where_obj(ch, arg, tmp_num);
	}

	if (!found
		&& tmp_num == num) {
		return false;
	} else {
		num = tmp_num;
		return true;
	}
}

void perform_mortal_where(CharData *ch, char *arg) {
	DescriptorData *d;

	SendMsgToChar("Кто много знает, тот плохо спит.\r\n", ch);
	return;

	if (!*arg) {
		SendMsgToChar("Игроки, находящиеся в зоне\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) != CON_PLAYING
				|| d->character.get() == ch) {
				continue;
			}

			const auto i = d->get_character();
			if (!i) {
				continue;
			}

			if (i->in_room == kNowhere
				|| !CAN_SEE(ch, i)) {
				continue;
			}

			if (world[ch->in_room]->zone_rn != world[i->in_room]->zone_rn) {
				continue;
			}

			sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			SendMsgToChar(buf, ch);
		}
	} else        // print only FIRST char, not all.
	{
		for (const auto &i : character_list) {
			if (i->in_room == kNowhere
				|| i.get() == ch) {
				continue;
			}

			if (!CAN_SEE(ch, i)
				|| world[i->in_room]->zone_rn != world[ch->in_room]->zone_rn) {
				continue;
			}

			if (!isname(arg, i->GetCharAliases())) {
				continue;
			}

			sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room]->name);
			SendMsgToChar(buf, ch);
			return;
		}
		SendMsgToChar("Никого похожего с этим именем нет.\r\n", ch);
	}
}

// возвращает true если объект был выведен
bool print_object_location(int num, const ObjData *obj, CharData *ch) {
	std::stringstream ss;
	if (num > 0) {
		ss << fmt::format("{:>2}. ", num);
		if (IS_GRGOD(ch)) {
			ss <<  fmt::format("[{:>7}] {:<25} - ", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
		} else {
			ss << fmt::format("{:<34} - ", obj->get_short_description().c_str());
		}
	} else {
		ss << fmt::format("{:>41}", " - ");
	}

	if (obj->get_in_room() > kNowhere) {
		ss << fmt::format("[{:>7}] {}", GET_ROOM_VNUM(obj->get_in_room()), world[obj->get_in_room()]->name);
		ss << "\r\n";
		SendMsgToChar(ss.str().c_str(), ch);
	} else if (obj->get_carried_by()) {
		ss << fmt::format("затарено {} [{}] в комнате [{}]", PERS(obj->get_carried_by(), ch, 4), GET_MOB_VNUM(obj->get_carried_by()),
						  world[obj->get_carried_by()->in_room]->vnum);
		ss << "\r\n";
		SendMsgToChar(ss.str().c_str(), ch);
	} else if (obj->get_worn_by()) {
		ss << fmt::format("надет на {} [{}] в комнате [{}]", PERS(obj->get_worn_by(), ch, 3), GET_MOB_VNUM(obj->get_worn_by()),
						  world[obj->get_worn_by()->in_room]->vnum);
		ss << "\r\n";
		SendMsgToChar(ss.str().c_str(), ch);
	} else if (obj->get_in_obj()) {
		if (Clan::is_clan_chest(obj->get_in_obj()))// || Clan::is_ingr_chest(obj->get_in_obj())) сделать отдельный поиск
		{
			return false; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
		} else {
			ss << fmt::format("лежит в [{}] {}, который находится \r\n",
							  GET_OBJ_VNUM(obj->get_in_obj()), obj->get_in_obj()->get_PName(5).c_str());
			SendMsgToChar(ss.str().c_str(), ch);
			print_object_location(0, obj->get_in_obj(), ch);
		}
	} else {
		for (ExchangeItem *j = exchange_item_list; j; j = j->next) {
			if (GET_EXCHANGE_ITEM(j)->get_unique_id() == obj->get_unique_id()) {
				ss << fmt::format("продается на базаре, лот #{}\r\n", GET_EXCHANGE_ITEM_LOT(j));
				SendMsgToChar(ss.str().c_str(), ch);
				return true;
			}
		}
		for (const auto &shop : GlobalObjects::Shops()) {
			const auto tmp_obj = shop->GetObjFromShop(obj->get_unique_id());
			if (!tmp_obj) {
				continue;
			}
			ss << fmt::format("можно купить в магазине: {}\r\n", shop->GetDictionaryName());
			SendMsgToChar(ss.str().c_str(), ch);
			return true;
		}
		if (obj->get_in_room() == kNowhere) {
			ss << "находится где-то там, далеко-далеко...\r\n";
			SendMsgToChar(ss.str().c_str(), ch);
		} else {
			ss << "расположение не найдено.\r\n";
			SendMsgToChar(ss.str().c_str(), ch);
		}
		return true;
	}

	return true;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
