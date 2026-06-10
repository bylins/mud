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
#include "engine/core/target_resolver.h"
#include "engine/ui/cmd/do_where.h"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <vector>
#include "gameplay/mechanics/sight.h"

void PerformImmortWhere(CharData *ch, char *arg);
void PerformMortalWhere(CharData *ch, char *arg);
bool PrintObjectLocation(int num, const ObjData *obj, CharData *ch);

static std::string ResolveSpecialLocation(const ObjData *obj);
static std::vector<std::string> ResolveObjLocationLines(const ObjData *obj, CharData *ch);
static bool CollectWhereObjects(CharData *ch, char *arg, int &num, std::vector<where_format::WhereRow> &rows);

void DoWhere(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);

	if (ch->IsGrGod() || ch->IsFlagged(EPrf::kCoderinfo))
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
				if (i && sight::CanSee(ch, i) && (i->in_room != kNowhere)) {
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
		std::vector<where_format::WhereRow> rows;
		target_resolver::Query q;
		q.scopes = {target_resolver::Scope::kWorld};
		q.name = arg;
		q.char_predicate = [](CharData *c) { return c->in_room != kNowhere; };
		for (CharData *i : target_resolver::ResolveChars(ch, q)) {
			ZoneData *zone = &zone_table[world[i->in_room]->zone_rn];
			found = 1;
			where_format::WhereRow row;
			row.num = num++;
			row.kind = i->IsNpc() ? where_format::ERowKind::kMob : where_format::ERowKind::kPlayer;
			row.vnum = GET_MOB_VNUM(i);
			row.name = GET_NAME(i);
			row.location_lines.push_back(fmt::format("[{:>7}] {}. Зона: '{}'",
					GET_ROOM_VNUM(i->in_room), world[i->in_room]->name, zone->name.c_str()));
			rows.push_back(std::move(row));
		}
		if (CollectWhereObjects(ch, arg, num, rows)) {
			found = 1;
		}
		if (found) {
			SendMsgToChar(where_format::FormatWhere(rows), ch);
		} else {
			SendMsgToChar("Нет ничего похожего.\r\n", ch);
		}
	}
}

/**
* Иммский поиск шмоток по 'где' с проходом как по глобальному списку, так
* и по спискам хранилищ и почты. Собирает найденное в rows, продолжая
* сквозную нумерацию из num; возвращает true, если что-то добавлено.
*/
static bool CollectWhereObjects(CharData *ch, char *arg, int &num, std::vector<where_format::WhereRow> &rows) {
	bool found = false;
	target_resolver::Query q;
	q.scopes = {target_resolver::Scope::kWorld};
	q.name = arg;
	q.visible_only = false;
	for (ObjData *obj : target_resolver::ResolveObjs(ch, q)) {
		where_format::WhereRow row;
		row.num = num++;
		row.kind = where_format::ERowKind::kObject;
		row.vnum = GET_OBJ_VNUM(obj);
		row.name = obj->get_short_description();
		row.location_lines = ResolveObjLocationLines(obj, ch);
		rows.push_back(std::move(row));
		found = true;
	}
	return found;
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
				|| !sight::CanSee(ch, i)) {
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

			if (!sight::CanSee(ch, i)
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

namespace {

const char *RowKindLabel(where_format::ERowKind kind) {
	switch (kind) {
		case where_format::ERowKind::kMob: return "Моб";
		case where_format::ERowKind::kPlayer: return "Игрок";
		default: return "Вещь";
	}
}

} // namespace

std::string where_format::FormatWhere(const std::vector<WhereRow> &rows) {
	int max_num = 0;
	for (const auto &row : rows) {
		max_num = std::max(max_num, row.num);
	}
	const int num_width = std::max(2, static_cast<int>(std::to_string(max_num).size()));

	std::string out;
	for (const auto &row : rows) {
		const std::string prefix = fmt::format("{:>{}}. {:<5} [{:>7}] {:<25} - ",
				row.num, num_width, RowKindLabel(row.kind), row.vnum, row.name);
		// Отступ строк-продолжений = длине префикса, чтобы разделитель " - "
		// встал ровно под разделителем первой строки.
		const std::string cont = fmt::format("{:>{}}", " - ", static_cast<int>(prefix.size()));

		out += prefix;
		if (!row.location_lines.empty()) {
			out += row.location_lines.front();
		}
		out += "\r\n";
		for (std::size_t i = 1; i < row.location_lines.size(); ++i) {
			out += cont;
			out += row.location_lines[i];
			out += "\r\n";
		}
	}
	return out;
}

// Спец-локации предмета (базар / магазин / клан / депот / почта / иначе).
// Возвращает одну строку без хвостового перевода строки.
static std::string ResolveSpecialLocation(const ObjData *obj) {
	for (ExchangeItem *j = exchange_item_list; j; j = j->next) {
		if (GET_EXCHANGE_ITEM(j)->get_unique_id() == obj->get_unique_id()) {
			return fmt::format("продается на базаре, лот #{}", GET_EXCHANGE_ITEM_LOT(j));
		}
	}
	for (const auto &shop : GlobalObjects::Shops()) {
		if (shop->GetObjFromShop(obj->get_unique_id())) {
			return fmt::format("можно купить в магазине: {}", shop->GetDictionaryName());
		}
	}
	std::string str = Clan::print_imm_where_obj(obj);
	if (str.empty()) {
		str = Depot::print_imm_where_obj(obj);
	}
	if (str.empty()) {
		str = Parcel::FindParcelObj(obj);
	}
	if (!str.empty()) {
		// эти помощники возвращают строку с хвостовым "\r\n" - срезаем его,
		// перевод строки добавит FormatWhere.
		while (!str.empty() && (str.back() == '\r' || str.back() == '\n')) {
			str.pop_back();
		}
		return str;
	}
	if (obj->get_in_room() == kNowhere) {
		return "находится где-то там, далеко-далеко...";
	}
	return "расположение не найдено.";
}

// Разворачивает локацию предмета в строки без рекурсии: для предмета внутри
// контейнера цепочка вложенности даёт несколько строк-продолжений.
static std::vector<std::string> ResolveObjLocationLines(const ObjData *obj, CharData *ch) {
	std::vector<std::string> lines;
	for (const ObjData *cur = obj;;) {
		if (cur->get_in_room() > kNowhere) {
			lines.push_back(fmt::format("[{:>7}] {}",
					GET_ROOM_VNUM(cur->get_in_room()), world[cur->get_in_room()]->name));
			return lines;
		}
		if (cur->get_carried_by()) {
			lines.push_back(fmt::format("затарено {} [{}] в комнате [{}]",
					sight::PersonName(cur->get_carried_by(), ch, 4), GET_MOB_VNUM(cur->get_carried_by()),
					world[cur->get_carried_by()->in_room]->vnum));
			return lines;
		}
		if (cur->get_worn_by()) {
			lines.push_back(fmt::format("надет на {} [{}] в комнате [{}]",
					sight::PersonName(cur->get_worn_by(), ch, 3), GET_MOB_VNUM(cur->get_worn_by()),
					world[cur->get_worn_by()->in_room]->vnum));
			return lines;
		}
		if (cur->get_in_obj() && !Clan::is_clan_chest(cur->get_in_obj())) {// || Clan::is_ingr_chest(cur->get_in_obj())) сделать отдельный поиск
			lines.push_back(fmt::format("лежит в [{}] {}, который находится",
					GET_OBJ_VNUM(cur->get_in_obj()), cur->get_in_obj()->get_PName(grammar::ECase::kPre)));
			cur = cur->get_in_obj();
			continue;
		}
		lines.push_back(ResolveSpecialLocation(cur));
		return lines;
	}
}

// возвращает true если объект был выведен
bool PrintObjectLocation(int num, const ObjData *obj, CharData *ch) {
	where_format::WhereRow row;
	row.num = num;
	row.kind = where_format::ERowKind::kObject;
	row.vnum = GET_OBJ_VNUM(obj);
	row.name = obj->get_short_description();
	row.location_lines = ResolveObjLocationLines(obj, ch);
	SendMsgToChar(where_format::FormatWhere({row}), ch);
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
						  (*it)->get_PName(grammar::ECase::kNom).c_str(), (*it)->get_vnum(), sum);
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
