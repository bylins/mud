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
#include "utils/utils_time.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/db/obj_prototypes.h"

#include <third_party_libs/fmt/include/fmt/format.h>

void PerformImmortWhere(CharData *ch, char *arg);
void PerformMortalWhere(CharData *ch, char *arg);
bool PrintImmWhereObj(CharData *ch, char *arg, int num);
bool PrintObjectLocation(int num, const ObjData *obj, CharData *ch);

void DoWhere(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);

	if (IS_GRGOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
		PerformImmortWhere(ch, arg);
	else
		PerformMortalWhere(ch, arg);
}

void PerformImmortWhere(CharData *ch, char *arg) {
	DescriptorData *d;
	int num = 1, found = 0;
	std::stringstream ss;
	if (!*arg) {
		ss << "ИГРОКИ\r\n------\r\n";
		for (d = descriptor_list; d; d = d->next) {
			if (d->state == EConState::kPlaying) {
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
		if (!PrintImmWhereObj(ch, arg, num) && !found) {
			SendMsgToChar("Нет ничего похожего.\r\n", ch);
		}
	}
}

/**
* Иммский поиск шмоток по 'где' с проходом как по глобальному списку, так
* и по спискам хранилищ и почты.
*/
bool PrintImmWhereObj(CharData *ch, char *arg, int num) {
	bool found = false;

	/* maybe it is possible to create some index instead of linear search */
	world_objects.foreach([&](const ObjData::shared_ptr& object) {
	  if (isname(arg, object->get_aliases())) {
		  if (PrintObjectLocation(num, object.get(), ch)) {
			  found = true;
			  num++;
		  }
	  }
	});

	if (found) {
		return true;
	}
	return false;
}

void PerformMortalWhere(CharData *ch, char *arg) {
	DescriptorData *d;

	SendMsgToChar("Кто много знает, тот плохо спит.\r\n", ch);
	return;

	if (!*arg) {
		SendMsgToChar("Игроки, находящиеся в зоне\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next) {
			if (d->state != EConState::kPlaying
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
bool PrintObjectLocation(int num, const ObjData *obj, CharData *ch) {
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
	} else if (obj->get_in_obj() && !Clan::is_clan_chest(obj->get_in_obj())) {// || Clan::is_ingr_chest(obj->get_in_obj())) сделать отдельный поиск
		ss << fmt::format("лежит в [{}] {}, который находится \r\n",
				GET_OBJ_VNUM(obj->get_in_obj()), obj->get_in_obj()->get_PName(ECase::kPre).c_str());
		SendMsgToChar(ss.str().c_str(), ch);
		PrintObjectLocation(0, obj->get_in_obj(), ch);
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
		std::string str = Clan::print_imm_where_obj(obj);

		if (!str.empty()) {
			ss << str;
			SendMsgToChar(ss.str().c_str(), ch);
			return true;
		}
		str = Depot::print_imm_where_obj(obj);
		if (!str.empty()) {
			ss << str;
			SendMsgToChar(ss.str().c_str(), ch);
			return true;
		}
		str = Parcel::FindParcelObj(obj);
		if (!str.empty()) {
			ss << str;
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

void FindErrorCountObj(CharData *ch) {
	int num = 1;
	size_t sum;
	utils::CExecutionTimer timer;
	ObjRnum start_dung = GetObjRnum(dungeons::kZoneStartDungeons * 100);

	auto it_start = std::find_if(obj_proto.begin(), obj_proto.end(), [start_dung] (auto it) {return (it->get_rnum() == start_dung); });

	for (auto it = it_start; it != obj_proto.end(); it++) {
		if ((*it)->get_parent_rnum() == -1)
			continue;
		std::list<ObjData *> objs;
		std::list<ObjData *> objs_orig;
		ObjRnum orn = (*it)->get_rnum();
		ObjRnum rnum = (*it)->get_parent_rnum();

		world_objects.GetObjListByRnum(orn, objs);
		sum = objs.size();
		std::for_each(it_start, obj_proto.end(), [&rnum, &sum, &objs, &it] (auto it2) {
		  if (it2 == *it)
			  return;
		  if (it2->get_parent_rnum() == rnum) {
			  if (CAN_WEAR(it2.get(), EWearFlag::kTake) && !it2->has_flag(EObjFlag::kQuestItem)) {
				  world_objects.GetObjListByRnum(it2->get_rnum(), objs);
				  sum += objs.size();
			  }
		  }
		});
		if (CAN_WEAR(obj_proto[rnum].get(), EWearFlag::kTake) && !obj_proto[rnum].get()->has_flag(EObjFlag::kQuestItem)) {
			world_objects.GetObjListByRnum((*it)->get_parent_rnum(), objs_orig);
			sum += objs_orig.size();
		}
		if (sum != (size_t)obj_proto.total_online(orn)) {
			SendMsgToChar(ch, "Найден предмет с ошибкой в реальном количестве %s #%d sum = %zu \r\n",
						  (*it)->get_PName(ECase::kNom).c_str(), (*it)->get_vnum(), sum);
			for (auto object : objs) {
				PrintObjectLocation(num++, object, ch);
			}
			for (auto object : objs_orig) {
				PrintObjectLocation(num++, object, ch);
			}
		}
	}
	SendMsgToChar(ch, "Поиск ошибок занял %f\r\n", timer.delta().count());
}

void DoFindObjByRnum(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjRnum orn;
	int num = 1;
	std::list<ObjData *> objs;

	one_argument(argument, buf);
	if (!str_cmp(buf, "error")) {
		FindErrorCountObj(ch);
		return;
	}
	if (!*buf || !a_isdigit(*buf)) {
		SendMsgToChar("Usage: objfind <rnum number> - найти предметы по RNUM\r\n", ch);
		return;
	}
	if ((orn = atoi(buf)) < 0 || (size_t)orn > (world_objects.size() - 1)) {
		SendMsgToChar("Указан неверный RNUM объекта !\r\n", ch);
		return;
	}
	world_objects.GetObjListByRnum(orn, objs);
	if (objs.empty()) {
		SendMsgToChar("Нет таких предметов в мире.\r\n", ch);
		return;
	}
	for (auto object : objs) {
		PrintObjectLocation(num++, object, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
