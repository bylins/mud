//
// Created by Sventovit on 03.09.2024.
//

#include "cmd/follow.h"
#include "db.h"
#include "dg_script/dg_db_scripts.h"
#include "game_mechanics/dungeons.h"
#include "handler.h"
#include "liquid.h"
#include "obj_prototypes.h"
#include "structs/global_objects.h"

#include "entities/char_data.h"

#include <third_party_libs/fmt/include/fmt/format.h>


extern void ExtractRepopDecayObject(const ObjData::shared_ptr &obj);

namespace dungeons {

const int kNumberOfZoneDungeons = 50;
const ZoneVnum kZoneStartDungeons = 30000;

struct ZrnComplexList {
  ZoneRnum from;
  ZoneRnum to;
};

std::string WhoInZone(ZoneRnum zrn);
ZoneVnum CheckDungionErrors(ZoneRnum zrn_from);

void RoomDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to, std::vector<ZrnComplexList> dungeon_list = {});
void MobDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to);
void TrigDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to);
void ObjDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to);
void ZoneDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to);
void RoomDataFree(ZoneRnum zrn);
void MobDataFree(ZoneRnum zrn);
void ObjDataFree(ZoneRnum zrn);
void TrigDataFree(ZoneRnum zrn);
void ZoneDataFree(ZoneRnum zrn);
void AddDungeonShopSeller(MobRnum mrn_from, MobRnum mrn_to);
void RemoveShopSeller(MobRnum mrn);
void ZoneTransformCMD(ZoneRnum zrn_to, ZoneRnum zrn_from);
void SwapOriginalObject(ObjData *obj);

void DoDungeonReset(CharData * /*ch*/, char *argument, int /*cmd*/, int /*subcmd*/) {
	DungeonReset(GetZoneRnum(atoi(argument)));
}
void DoZoneCopy(CharData *, char *argument, int, int) {
	ZoneCopy(atoi(argument));
}

void TrigCommandsConvert(ZoneRnum zrn_from, ZoneRnum zrn_to, ZoneRnum replacer_zrn) {
	TrgRnum trn_start = zone_table[zrn_to].RnumTrigsLocation.first;
	TrgRnum trn_stop = zone_table[zrn_to].RnumTrigsLocation.second;
	std::string replacer = to_string(zone_table[replacer_zrn].vnum);
	std::string search = to_string(zone_table[zrn_from].vnum);

	if (zone_table[zrn_from].vnum < 100) {
		sprintf(buf, "Номер зоны меньше 100, текст триггера не изменяется!");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return;
	}
	for(int i = trn_start; i <= trn_stop; i++) {
		auto c = *trig_index[i]->proto->cmdlist;

		while (c) {
			utils::ReplaceAll(c->cmd, search, replacer);
			c = c->next;
		}
	}
}

ZoneRnum ZoneCopy(ZoneVnum zvn_from) {
	ZoneVnum zvn_to;
	RoomRnum rnum_start, rnum_stop;
	ZoneRnum zrn_from = GetZoneRnum(zvn_from);

	if (!GetZoneRooms(zrn_from, &rnum_start, &rnum_stop)) {
		auto msg = fmt::format("Нет комнат в зоне {}.", zvn_from);
		mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	if (world[rnum_start]->vnum % 100 != 0) {
		auto msg = fmt::format("Нет 00 комнаты в зоне источнике {}.", zvn_from);
		mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
	}
	for (zvn_to = kZoneStartDungeons; zvn_to < kZoneStartDungeons + kNumberOfZoneDungeons; zvn_to++) {
		if (zone_table[GetZoneRnum(zvn_to)].copy_from_zone == 0) {
			auto msg = fmt::format("Клонирую зону {} в {}, осталось мест: {}",
					zvn_from, zvn_to, kZoneStartDungeons + kNumberOfZoneDungeons - zvn_to - 1);
			mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
			break;
		}
	}
	if (zvn_to == kZoneStartDungeons + kNumberOfZoneDungeons) {
		mudlog("Нет свободного места.", CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	if (zvn_from < 100) {
		mudlog("Попытка склонировать двухзначную зону.", CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	if (zvn_from >= kZoneStartDungeons) {
		mudlog("Попытка склонировать данж.", CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	ZoneRnum zrn_to = GetZoneRnum(zvn_to);

	if (zrn_to == 0) {
		mudlog("Нет такой зоны.", CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	utils::CExecutionTimer timer;
	auto msg = fmt::format("Попытка создать dungeon, zone {} {}",
						   zone_table[zrn_to].name.c_str(), zone_table[zrn_to].vnum);
	mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
	TrigDataCopy(zrn_from, zrn_to);
	TrigCommandsConvert(zrn_from, zrn_to, zrn_to);
	RoomDataCopy(zrn_from, zrn_to);
	MobDataCopy(zrn_from, zrn_to);
	ObjDataCopy(zrn_from, zrn_to);
	ZoneDataCopy(zrn_from, zrn_to); //последним
	msg = fmt::format("Сбрасываю зону {}, delta {:.3f}", zone_table[zrn_to].vnum, timer.delta().count());
	mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
	ResetZone(zrn_to);
	zone_table[zrn_to].copy_from_zone = zone_table[zrn_from].vnum;
//	zone_table[zrn_to].under_construction = true;
	msg = fmt::format("Create dungeon, zone {} {}, delta {:.3f}",
					  zone_table[zrn_to].name.c_str(), zone_table[zrn_to].vnum, timer.delta().count());
	mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
	return zrn_to;
}

void CreateBlankZoneDungeon() {
	ZoneVnum zone_vnum = kZoneStartDungeons;

	for (ZoneVnum zvn = 0; zvn < kNumberOfZoneDungeons; zvn++) {
		ZoneData new_zone;

		new_zone.vnum = zone_vnum;
		new_zone.name = "Зона для данжей";
		new_zone.under_construction = true;
		new_zone.top = zone_vnum * 100 + 99;
		new_zone.cmd = nullptr; //[0].command = 'S'; //пустой список команд
		zone_table.push_back(std::move(new_zone));
		zone_vnum++;
	}
}

void CreateBlankRoomDungeon() {
	ZoneVnum zone_vnum = kZoneStartDungeons;
	ZoneRnum zone_rnum = GetZoneRnum(kZoneStartDungeons);

	for (ZoneVnum zvn = 0; zvn < kNumberOfZoneDungeons; zvn++) {
		for (RoomVnum rvn = 0; rvn <= 98; rvn++) {
			auto *new_room = new RoomData;

			top_of_world++;
			world.push_back(new_room);
			new_room->zone_rn = zone_rnum;
			new_room->vnum = zone_vnum * 100 + rvn;
//			log("Room rnum %d vnum %d zone %d (%d), in zone %d", GetRoomRnum(new_room->vnum), new_room->vnum, zone_rnum, zone_vnum, zone_table[zone_rnum].vnum);
			new_room->sector_type = ESector::kSecret;
			new_room->name = str_dup("ДАНЖ");
		}
		zone_vnum++;
		zone_rnum++;
	}
}

void CreateBlankTrigsDungeon() {
	IndexData **new_index;
	size_t size_new_trig_table = (top_of_trigt - 1) + 100 * kNumberOfZoneDungeons;

	CREATE(new_index, size_new_trig_table + 1);
	for (int i = 0; i < top_of_trigt; i++) {
		new_index[i] = trig_index[i];
	}
	for (ZoneVnum zvn = kZoneStartDungeons; zvn <= kZoneStartDungeons + (kNumberOfZoneDungeons - 1); zvn++) {
		zone_table[GetZoneRnum(zvn)].RnumTrigsLocation.first = top_of_trigt;
		zone_table[GetZoneRnum(zvn)].RnumTrigsLocation.second = top_of_trigt + 99;
		for (TrgVnum tvn = 0; tvn <= 99; tvn++) {
			auto *trig = new Trigger(top_of_trigt, "Blank trigger", MTRIG_GREET);
			IndexData *index;
			CREATE(index, 1);
			index->vnum = zvn * 100 + tvn;
			index->total_online = 0;
			index->func = nullptr;
			index->proto = trig;
			new_index[top_of_trigt++] = index;
		}
	}
	free(trig_index);
	trig_index = new_index;
}

void CreateBlankObjsDungeon() {
	ObjVnum obj_vnum;
	for (ZoneVnum zvn = kZoneStartDungeons; zvn <= kZoneStartDungeons + (kNumberOfZoneDungeons - 1); zvn++) {
		for (ObjVnum vnum = 0; vnum <= 99; vnum++) {
			ObjData *obj;

			obj_vnum = vnum + zvn * 100;
			NEWCREATE(obj, obj_vnum);
			obj->set_aliases("новый предмет");
			obj->set_description("что-то новое лежит здесь");
			obj->set_short_description("новый предмет");
			obj->set_PName(0, "это что");
			obj->set_PName(1, "нету чего");
			obj->set_PName(2, "привязать к чему");
			obj->set_PName(3, "взять что");
			obj->set_PName(4, "вооружиться чем");
			obj->set_PName(5, "говорить о чем");
			obj->set_wear_flags(to_underlying(EWearFlag::kTake));
			obj_proto.add(obj, obj_vnum);
		}
	}
}

void CreateBlankMobsDungeon() {
	CharData *new_proto;
	IndexData *new_index;
	size_t size_new_mob_table = top_of_mobt + 100 * kNumberOfZoneDungeons;
	new_proto = new CharData[size_new_mob_table + 1];
	CREATE(new_index, size_new_mob_table + 1);

	for (int i = 0; i <= top_of_mobt; i++) {
//		log("copyng mobs top %d i %d size %ld",top_of_mobt, i, size_new_mob_table );
		new_proto[i] = mob_proto[i];
		new_index[i] = mob_index[i];
	}
	MobRnum rnum = top_of_mobt + 1;

	for (ZoneVnum zvn = kZoneStartDungeons; zvn <= kZoneStartDungeons + (kNumberOfZoneDungeons - 1); zvn++) {
		zone_table[GetZoneRnum(zvn)].RnumMobsLocation.first = rnum;
		for (MobVnum mvn = 0; mvn <= 99; mvn++) {
			new_proto[rnum].set_rnum(rnum);
			new_index[rnum].vnum = mvn + zvn * 100;
			new_proto[rnum].set_npc_name("пустой моб");
			new_proto[rnum].SetCharAliases("моб");
			new_proto[rnum].player_data.PNames[0] = "пустой моб";
			new_index[rnum].total_online = 0;
			new_index[rnum].stored = 0;
			new_index[rnum].func = nullptr;
			new_proto[rnum].script->cleanup();
			new_proto[rnum].proto_script->clear();
			new_index[rnum].zone = zvn;
			new_proto[rnum].SetFlag(EMobFlag::kNpc);
			new_proto[rnum].player_specials = player_special_data::s_for_mobiles;
			new_index[rnum].set_idx = -1;
			top_of_mobt++;
			rnum++;
		}
		zone_table[GetZoneRnum(zvn)].RnumMobsLocation.second = rnum - 1;
	}

	delete[] mob_proto;
	free(mob_index);
	mob_proto = new_proto;
	mob_index = new_index;

}

void ZoneDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to) {
	int count, subcmd;
	auto &zone_from = zone_table[zrn_from];
	auto &zone_to = zone_table[zrn_to];

	zone_to.name = zone_from.name;
	zone_to.entrance = zone_to.vnum * 100 + zone_from.entrance % 100;
	zone_to.comment = zone_from.comment;
	zone_to.location = zone_from.location;
	zone_to.author = zone_from.author;
	zone_to.description = zone_from.description;
	zone_to.level = zone_from.level;
	zone_to.mob_level = zone_from.mob_level;
	zone_to.type = zone_from.type;
	zone_to.top = zone_to.vnum * 100 + 99;
	zone_to.reset_mode = 0; //самостоятельно не ресетится
	zone_to.lifespan = zone_from.lifespan;
	zone_to.reset_idle = true;
	zone_to.typeA_count = 0;
	zone_to.typeB_count = 0;
	zone_to.under_construction = zone_from.under_construction;
	zone_to.locked = zone_from.locked;
	zone_to.group = zone_from.group;
/*
	if (zone_to.typeA_count) {
		CREATE(zone_to.typeA_list, zone_to.typeA_count); //почистить
	}
	for (i = 0; i < zone_to.typeA_count; i++) {
		for (auto d_zvn = kZoneStartDungeons; d_zvn < kZoneStartDungeons + kNumberOfZoneDungeons; d_zvn++) {
			if (d_zvn == zone_from.typeA_list[i]) {
				zone_to.typeA_list[i] = d_zvn;
			}
		}
	}
	if (zone_to.typeB_count) {
		CREATE(zone_to.typeB_list, zone_to.typeB_count); //почистить
		CREATE(zone_to.typeB_flag, zone_to.typeB_count); //почистить
	}
	for (i = 0; i < zone_to.typeB_count; i++) {
		for (auto d_zvn = kZoneStartDungeons; d_zvn < kZoneStartDungeons + kNumberOfZoneDungeons; d_zvn++) {
			if (d_zvn == zone_from.typeB_list[i]) {
				zone_to.typeB_list[i] = d_zvn;
			}
		}
	}
*/
	if (zone_from.cmd) {
		for (count = 0; zone_from.cmd[count].command != 'S'; ++count);
		CREATE(zone_to.cmd, count + 1); //почистить
		for (subcmd = 0; zone_from.cmd[subcmd].command != 'S'; ++subcmd) {
			zone_to.cmd[subcmd].command = zone_from.cmd[subcmd].command;
			zone_to.cmd[subcmd].if_flag = zone_from.cmd[subcmd].if_flag;
			zone_to.cmd[subcmd].arg1 = zone_from.cmd[subcmd].arg1;
			zone_to.cmd[subcmd].arg2 = zone_from.cmd[subcmd].arg2;
			zone_to.cmd[subcmd].arg3 = zone_from.cmd[subcmd].arg3;
			zone_to.cmd[subcmd].arg4 = zone_from.cmd[subcmd].arg4;
			if (zone_from.cmd[subcmd].sarg1) {
				zone_to.cmd[subcmd].sarg1 = str_dup(zone_from.cmd[subcmd].sarg1); //почистить
			}
			if (zone_from.cmd[subcmd].sarg2) {
				zone_to.cmd[subcmd].sarg1 = str_dup(zone_from.cmd[subcmd].sarg2); //почистить
			}
		}
		zone_to.cmd[subcmd].command = 'S';
	}
	ZoneTransformCMD(zrn_to, zrn_from);

/*
	for (subcmd = 0; zone_from.cmd[subcmd].command != 'S'; ++subcmd) {
		log("CMD %d %d %d %d %d %d",
		zone_from.cmd[subcmd].command, zone_to.cmd[subcmd].if_flag,
		zone_to.cmd[subcmd].arg1, zone_to.cmd[subcmd].arg2,
		zone_to.cmd[subcmd].arg3, zone_to.cmd[subcmd].arg4);
	}
*/
}

void RoomDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to, std::vector<ZrnComplexList> dungeon_list) {
	RoomRnum rrn_start = zone_table[zrn_from].RnumRoomsLocation.first;
	RoomRnum rrn_stop = zone_table[zrn_from].RnumRoomsLocation.second;
	RoomRnum rrn_to = zone_table[zrn_to].RnumRoomsLocation.first;

	if (rrn_start == -1) {
		mudlog("В зоне нет комнат, копируем остальное", CMP, kLvlGreatGod, SYSLOG, true);
		return;
	}
/* уже не нужна чистка комнаты реальны
	for (auto room = rrn_to; room <= rrn_to + 98; room++) {
		auto people_copy = world[room]->people;

		for (const auto vict : people_copy) {
			if (vict->IsNpc()) {
				if (vict->followers
					|| vict->has_master()) {
					die_follower(vict);
				}
				if (!vict->purged()) {
					ExtractCharFromWorld(vict, false);
				}
			} else {
				RemoveCharFromRoom(vict);
				PlaceCharToRoom(vict, GetRoomRnum(calc_loadroom(vict)));
				look_at_room(vict, GetRoomRnum((calc_loadroom(vict)));
			}
		}
		ObjData *obj, *next_o;

		for ( obj = world[room]->contents; obj; obj = next_o) {
			next_o = obj->get_next_content();
			ExtractObjFromWorld(obj);
		}
	}
*/
	for (int i = rrn_start; i <= rrn_stop; i++) {
		RoomRnum new_rnum = world[i]->vnum % 100 + rrn_to;
		auto &new_room = world[new_rnum];

		free(new_room->name);
		new_room->vnum = zone_table[zrn_to].vnum * 100 + world[i]->vnum % 100;
		new_room->name = str_dup(world[i]->name); //почистить
		new_room->description_num = world[i]->description_num;
		new_room->write_flags(world[i]->read_flags());
		new_room->sector_type = world[i]->sector_type;
		new_room->people.clear();
		new_room->func = nullptr;
		new_room->contents = nullptr;
		new_room->track = nullptr;
		new_room->light = 0;
		new_room->fires = 0;
		new_room->gdark = 0;
		new_room->glight = 0;
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			const auto &from = world[i]->dir_option[dir];
			if (from) {
				RoomVnum rvn = 0;
				int to_room = from->to_room();// - rrn_start + first_room_dungeon;

				if (to_room >= rrn_start && to_room <= rrn_stop) {
					rvn = zone_table[zrn_to].vnum * 100 + world[from->to_room()]->vnum % 100;
				} else {
					if (!dungeon_list.empty()) {
						auto zrn_to_it = std::find_if(dungeon_list.begin(),
													  dungeon_list.end(),
													  [&to_room](auto it) {
														return it.from == world[to_room]->zone_rn;
													  });
						if (zrn_to_it != dungeon_list.end()) {
							rvn = zone_table[zrn_to_it->to].vnum * 100 + world[from->to_room()]->vnum % 100;
						}
					}
				}
				new_room->dir_option[dir] = std::make_shared<ExitData>();
				new_room->dir_option[dir]->to_room(GetRoomRnum(rvn));
				if (!from->general_description.empty()) {
					new_room->dir_option[dir]->general_description = from->general_description; //чиcтить
				}
				if (from->keyword) {
					new_room->dir_option[dir]->set_keyword(from->keyword); //чистить
				}
				if (from->vkeyword) {
					new_room->dir_option[dir]->set_vkeyword(from->vkeyword); //чистить
				}
				new_room->dir_option[dir]->exit_info = from->exit_info;
				new_room->dir_option[dir]->key = zone_table[zrn_to].vnum * 100 + from->key % 100;
				new_room->dir_option[dir]->lock_complexity = from->lock_complexity;
			}
		}
		new_room->proto_script = std::make_shared<ObjData::triggers_list_t>();
		for (const auto trigger_vnum : *world[i]->proto_script) {
			if (zone_table[zrn_from].vnum == trigger_vnum / 100) {
				TrgVnum tvn = zone_table[zrn_to].vnum * 100 + trigger_vnum % 100;
				Trigger *trig = read_trigger(GetTriggerRnum(tvn));
				add_trigger(SCRIPT(new_room).get(), trig, -1);
				add_trig_to_owner(-1, tvn, new_room->vnum);
			} else {
				Trigger *trig = read_trigger(GetTriggerRnum(trigger_vnum));

				add_trigger(SCRIPT(new_room).get(), trig, -1);
				add_trig_to_owner(-1, trigger_vnum, new_room->vnum);
			}
		}
		ExtraDescription::shared_ptr sdd = world[i]->ex_description;
		while (sdd) {
			const ExtraDescription::shared_ptr new_descr(new ExtraDescription);
			new_descr->set_keyword(sdd->keyword);
			new_descr->set_description(sdd->description);
			new_descr->next = new_room->ex_description;
			sdd = sdd->next;
		}
	}
}

void ObjDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to) {
	ObjRnum orn_to, i;
	ObjVnum new_ovn;
	ObjData *new_obj;

	for (int counter = zone_table[zrn_from].vnum * 100; counter <= zone_table[zrn_from].top; counter++) {
		if ((i = GetObjRnum(counter)) >= 0) {
			new_ovn = zone_table[zrn_to].vnum * 100 + obj_proto[i]->get_vnum() % 100;
			orn_to = GetObjRnum(new_ovn);
			NEWCREATE(new_obj, new_ovn);
			const auto obj_original = world_objects.create_from_prototype_by_rnum(i);
			new_obj->copy_from(obj_original.get());
			if (new_obj->get_type() == EObjType::kLiquidContainer) {
				name_from_drinkcon(new_obj);
			}
			new_obj->set_parent_rnum(obj_original->get_rnum());
//			new_obj->set_extra_flag(EObjFlag::kNolocate);
//			new_obj->set_extra_flag(EObjFlag::kNorent);
			new_obj->set_extra_flag(EObjFlag::kNosell);
			obj_proto.replace(new_obj, orn_to, new_ovn);
			for (const auto tvn : obj_proto[i]->get_proto_script()) {
				if (zone_table[zrn_from].vnum == tvn / 100) {
					new_obj->add_proto_script(zone_table[zrn_to].vnum * 100 + tvn % 100);
					add_trig_to_owner(-1, zone_table[zrn_to].vnum * 100 + tvn % 100, new_obj->get_vnum());
				} else {
					new_obj->add_proto_script(tvn);
					add_trig_to_owner(-1, tvn, new_obj->get_vnum());
				}
			}
			if (new_obj->get_type() == EObjType::kContainer) {
				ObjVnum key = obj_original->get_val(2);
				ObjVnum new_key = zone_table[zrn_to].vnum * 100 + key % 100;

				new_obj->set_val(2, new_key);
			}
			ExtractObjFromWorld(obj_original.get());
		}
	}
}

void MobDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to) {
	MobRnum mrn_from = zone_table[zrn_from].RnumMobsLocation.first;
	MobRnum mrn_last = zone_table[zrn_from].RnumMobsLocation.second;
	MobRnum rrn_first = zone_table[zrn_to].RnumMobsLocation.first;

	if (mrn_from == -1) {
		sprintf(buf, "В зоне нет мобов, копируем остальное");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return;
	}
	for (int i = mrn_from; i <= mrn_last; i++) {
		MobRnum mrn_to = rrn_first + mob_index[i].vnum % 100;
		auto old_rnum = mob_proto[mrn_to].get_rnum();
		mob_proto[mrn_to] = mob_proto[i];
		mob_proto[mrn_to].set_rnum(old_rnum);
		mob_index[mrn_to] = mob_index[i];
		mob_index[mrn_to].vnum = zone_table[zrn_to].vnum * 100 + mob_index[i].vnum % 100;
		if (mob_index[i].func == shop_ext) {
			AddDungeonShopSeller(i, mrn_to);
		}
		mob_proto[mrn_to].script->cleanup();
		mob_proto[mrn_to].proto_script = std::make_shared<ObjData::triggers_list_t>();
		if (!mob_proto[i].summon_helpers.empty()) {
			mob_proto[mrn_to].summon_helpers.clear();
			for (const auto helper : mob_proto[i].summon_helpers) {
				if (zone_table[zrn_from].vnum == helper / 100) {
					mob_proto[mrn_to].summon_helpers.push_back(zone_table[zrn_to].vnum * 100 + helper % 100);
				} else {
					mob_proto[mrn_to].summon_helpers.push_back(helper);
				}
			}
		}
		for (const auto trigger_vnum : *mob_proto[i].proto_script) {
			if (zone_table[zrn_from].vnum == trigger_vnum / 100) {
				mob_proto[mrn_to].proto_script->push_back(zone_table[zrn_to].vnum * 100 + trigger_vnum % 100);
				add_trig_to_owner(-1, zone_table[zrn_to].vnum * 100 + trigger_vnum % 100, mob_index[mrn_to].vnum);
			} else {
				mob_proto[mrn_to].proto_script->push_back(trigger_vnum);
				add_trig_to_owner(-1, trigger_vnum, mob_index[mrn_to].vnum);
			}
		}
		mob_index[mrn_to].total_online = 0;
		mob_index[mrn_to].stored = 0;
	}
}

void TrigDataCopy(ZoneRnum zrn_from, ZoneRnum zrn_to) {
	TrgRnum trn_start = zone_table[zrn_from].RnumTrigsLocation.first;
	TrgRnum trn_stop = zone_table[zrn_from].RnumTrigsLocation.second;
	ZoneVnum zvn_to = zone_table[zrn_to].vnum;

	if (trn_start == -1) {
		mudlog("В зоне нет триггеров, копируем остальное", CMP, kLvlGreatGod, SYSLOG, true);
		return;
	}
	for (int i = trn_start; i <= trn_stop; i++) {
		auto *trig = new Trigger(*trig_index[i]->proto);
		TrgRnum new_tvn = trig_index[i]->vnum % 100 + zvn_to * 100;
		TrgRnum new_trn = GetTriggerRnum(new_tvn);

		trig->set_rnum(new_trn);
		trig->cmdlist = std::make_shared<cmdlist_element::shared_ptr>();
		*trig->cmdlist = std::make_shared<cmdlist_element>();
		auto c_copy = *trig->cmdlist;
		auto c = *trig_index[i]->proto->cmdlist;

		while (c) {
			c_copy->cmd = c->cmd;
			c_copy->line_num = c->line_num;
			c = c->next;
			if (c) {
				c_copy->next = std::make_shared<cmdlist_element>();
				c_copy = c_copy->next;
			}
		}
		trig_index[new_trn]->proto = trig;
	}
}

void ListDungeons(CharData *ch) {
	std::ostringstream buffer;
	ZoneRnum zrn_start = GetZoneRnum(kZoneStartDungeons);
	ZoneRnum zrn_stop = GetZoneRnum(kZoneStartDungeons + kNumberOfZoneDungeons - 1);

	buffer << fmt::format("{:>3}  {:>7} [{:>9}] {:<50} {:<}\r\n", "#", "предок", "вход", "название зоны", "игроки");
	for (int i = zrn_start; i <= zrn_stop; i++) {
		if (zone_table[i].copy_from_zone > 0)
			buffer << fmt::format("{:>3}) {:>7} [{:>9}] {:<50} {:<}\r\n",
								  i - zrn_start + 1,
								  zone_table[i].copy_from_zone,
								  zone_table[i].entrance,
								  zone_table[i].name,
								  WhoInZone(i));
	}
	SendMsgToChar(buffer.str().c_str(), ch);
}

std::string WhoInZone(ZoneRnum zrn) {
	int from = 0, to = 0;
	std::string pc;

	GetZoneRooms(zrn, &from, &to);
	for (auto d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING)
			continue;
		if (d->character->in_room >= from && d->character->in_room <= to) {
			pc += d->character->get_name() + " ";
		}
	}
	return pc.empty() ? "никого" : pc;
}

std::string CreateComplexDungeon(Trigger *trig, const std::vector<std::string> &tokens) {
	std::vector<ZrnComplexList> zrn_list;
	ZoneVnum zvn;
	ZoneRnum zrn;
	std::stringstream out_from;
	std::string out_to;
	ZrnComplexList pair{};

	for (const auto &it : tokens) {
		zrn = GetZoneRnum(stoi(it));
		if (zrn == 0) {
			trig_log(trig, fmt::format("Ошибка в создании комплекса, данж {} не найден", it).c_str());
			return "0";
		}
		zvn = CheckDungionErrors(zrn);
		if (zvn > 0) {
			pair.from = zrn;
			pair.to = GetZoneRnum(zvn);
			zrn_list.push_back(pair);
		} else
			return "0";
		out_from << it << " ";
	}
	utils::CExecutionTimer timer;
	trig_log(trig, fmt::format("Попытка создать комплекс: {}", out_from.str()).c_str());
	for (auto it : zrn_list) {
		out_to += to_string(zone_table[it.to].vnum) + " ";
		TrigDataCopy(it.from, it.to);
		RoomDataCopy(it.from, it.to, zrn_list);
		MobDataCopy(it.from, it.to);
		ObjDataCopy(it.from, it.to);
		ZoneDataCopy(it.from, it.to); //последним
		zone_table[it.to].copy_from_zone = zone_table[it.from].vnum;
	}
	out_to.pop_back();
	for (auto it : zrn_list) {
		for (auto it2 : zrn_list) {
			TrigCommandsConvert(it.from, it2.to, it.to);
		}
		ResetZone(it.to);
	}
	sprintf(buf, "Создан комплекс,  зоны %s delta %f", out_to.c_str(), timer.delta().count());
	mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);

	return out_to;
}

ZoneVnum CheckDungionErrors(ZoneRnum zrn_from) {
	ZoneVnum zvn_to;
	RoomRnum rnum_start, rnum_stop;
	ZoneVnum zvn_from = zone_table[zrn_from].vnum;

	if (!GetZoneRooms(zrn_from, &rnum_start, &rnum_stop)) {
		sprintf(buf, "Нет комнат в зоне %d.", static_cast<int>(zvn_from));
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	if (world[rnum_start]->vnum % 100 != 0) {
		sprintf(buf, "Нет 00 комнаты в зоне источнике %d", zvn_from);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
	if (zvn_from < 100) {
		sprintf(buf, "Попытка склонировать двухзначную зону.");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	if (zvn_from >= kZoneStartDungeons) {
		sprintf(buf, "Попытка склонировать данж.");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	for (zvn_to = kZoneStartDungeons; zvn_to < kZoneStartDungeons + kNumberOfZoneDungeons; zvn_to++) {
		if (zone_table[GetZoneRnum(zvn_to)].copy_from_zone == 0) {
			zone_table[GetZoneRnum(zvn_to)].copy_from_zone = zvn_to;
			sprintf(buf,
					"Клонирую зону %d в %d, осталось мест: %d",
					zvn_from,
					zvn_to,
					kZoneStartDungeons + kNumberOfZoneDungeons - zvn_to - 1);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
			break;
		}
	}
	if (zvn_to == kZoneStartDungeons + kNumberOfZoneDungeons) {
		sprintf(buf, "Нет свободного места.");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return 0;
	}
	return zvn_to;
}

void DungeonReset(int zrn) {
	utils::CExecutionTimer timer;
	log("copy_from_zone %d", zone_table[zrn].copy_from_zone);

	if (zrn == 0) {
		sprintf(buf, "Неправильный номер зоны");
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		return;
	}
	if (zone_table[zrn].copy_from_zone > 0) {
		utils::CExecutionTimer timer1;
		RoomDataFree(zrn);
		sprintf(buf,
				"Free rooms. zone %s %d, delta %f",
				zone_table[zrn].name.c_str(),
				zone_table[zrn].vnum,
				timer1.delta().count());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		utils::CExecutionTimer timer2;
		MobDataFree(zrn);
		sprintf(buf,
				"Free mobs. zone %s %d, delta %f",
				zone_table[zrn].name.c_str(),
				zone_table[zrn].vnum,
				timer2.delta().count());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		utils::CExecutionTimer timer4;
		ObjDataFree(zrn);
		sprintf(buf,
				"Free objs. zone %s %d, delta %f",
				zone_table[zrn].name.c_str(),
				zone_table[zrn].vnum,
				timer4.delta().count());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		utils::CExecutionTimer timer3;
		ZoneDataFree(zrn);
		sprintf(buf,
				"Free zone data. zone %s %d, delta %f",
				zone_table[zrn].name.c_str(),
				zone_table[zrn].vnum,
				timer3.delta().count());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		TrigDataFree(zrn);
		sprintf(buf,
				"Free all dungeons %s %d, delta %f",
				zone_table[zrn].name.c_str(),
				zone_table[zrn].vnum,
				timer.delta().count());
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		zone_table[zrn].copy_from_zone = 0;
		return;
	} else {
		sprintf(buf, "Попытка сбросить обычныю зону: %d", zone_table[zrn].vnum);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
}

void RoomDataFree(ZoneRnum zrn) {
	RoomRnum rrn_start = zone_table[zrn].RnumRoomsLocation.first;

	for (RoomVnum rrn = rrn_start; rrn <= rrn_start + 99; rrn++) {
		while (room_spells::IsRoomAffected(world[rrn], ESpell::kPortalTimer)) {
			RemovePortalGate(rrn);
		}
	}
	for (RoomVnum rvn = 0; rvn <= 99; rvn++) {
		auto &room = world[rrn_start + rvn];
		auto people_copy = room->people;

		for (const auto vict : people_copy) {
			if (vict->IsNpc()) {
				if (vict->followers
					|| vict->has_master()) {
					die_follower(vict);
				}
				if (!vict->purged()) {
					ExtractCharFromWorld(vict, false);
				}
			} else {
				RemoveCharFromRoom(vict);
				PlaceCharToRoom(vict, GetRoomRnum(calc_loadroom(vict)));
				look_at_room(vict, GetRoomRnum(calc_loadroom(vict)));
			}
		}
		ObjData *obj, *next_o;

		for (obj = room->contents; obj; obj = next_o) {
			next_o = obj->get_next_content();
			ExtractObjFromWorld(obj);
		}
		room->people.clear();
		free(room->name);
		room->name = str_dup("ДАНЖ!");
		room->cleanup_script();
		room->affected.clear();
		room->vnum = zone_table[zrn].vnum * 100 + rvn;
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
			if (room->dir_option[dir]) {
//				room->dir_option[dir]->general_description.clear();
//				room->dir_option[dir]->set_keyword("");
//				room->dir_option[dir]->set_vkeyword("");
				room->dir_option[dir].reset();
			}
		}
		ExtraDescription::shared_ptr sdd = room->ex_description;
		if (sdd) {
			while (sdd) {
				free(sdd->keyword);
				free(sdd->description);
				sdd = sdd->next;
			}
			sdd.reset();
		}
	}
}

void MobDataFree(ZoneRnum zrn) {
	MobRnum mrn_start = zone_table[zrn].RnumMobsLocation.first;
	ZoneVnum zvn = zone_table[zrn].vnum;

	for (MobRnum mrn = 0; mrn <= 99; mrn++) {
		if (mob_index[mrn_start + mrn].func == shop_ext) {
			RemoveShopSeller(mrn_start + mrn);
		}
		mob_proto[mrn_start + mrn].proto_script->clear();
		mob_proto[mrn_start + mrn].set_npc_name("пустой моб");
		mob_index[mrn_start + mrn].vnum = mrn + zvn * 100;
	}
// мобы удаляются в RoomDataFree
}

void ObjDataFree(ZoneRnum zrn) {
// на земле удаляются в RoomDataFree
//	ObjRnum orn_from = zone_table[zrn].RnumObjsLocation.first;
//	ObjRnum orn_last = zone_table[zrn].RnumObjsLocation.second;
	ObjRnum orn;

	for (int counter = zone_table[zrn].vnum * 100; counter <= zone_table[zrn].top; counter++) {
		if ((orn = GetObjRnum(counter)) >= 0) {
			obj_proto[orn]->clear_proto_script();
// перенесено в char_from_room
//			world_objects.foreach_with_rnum(orn, [&](const ObjData::shared_ptr &obj) {
//				SwapOriginalObject(obj);
//			});
			auto &obj = obj_proto[orn];
			obj->set_aliases("новый предмет");
			obj->set_description("что-то новое лежит здесь");
			obj->set_short_description("новый предмет");
			obj->set_PName(0, "это что");
			obj->set_PName(1, "нету чего");
			obj->set_PName(2, "привязать к чему");
			obj->set_PName(3, "взять что");
			obj->set_PName(4, "вооружиться чем");
			obj->set_PName(5, "говорить о чем");
			obj->set_wear_flags(to_underlying(EWearFlag::kTake));
			obj->set_parent_rnum(-1);
			obj->clear_proto_script();
		}
	}
	world_objects.foreach_on_copy([&zrn](const ObjData::shared_ptr &j) {
	  if (j->has_flag(EObjFlag::kRepopDecay) && j->get_vnum() / 100 == zone_table[zrn].vnum) {
		  ExtractRepopDecayObject(j);
	  }
	});
}

void TrigDataFree(ZoneRnum zrn) {
	TrgRnum rrn_start = zone_table[zrn].RnumTrigsLocation.first;
	ZoneVnum zvn = zone_table[zrn].vnum;

	for (TrgRnum trn = 0; trn <= 99; trn++) {
		trig_index[rrn_start + trn]->proto->set_name("Blank trigger");
		trig_index[rrn_start + trn]->proto->cmdlist->reset();
		trig_index[rrn_start + trn]->vnum = zvn * 100 + trn;
		owner_trig[trig_index[rrn_start + trn]->vnum].clear();
	}
}

void ZoneDataFree(ZoneRnum zrn) {
	for (int subcmd = 0; zone_table[zrn].cmd[subcmd].command != 'S'; ++subcmd) {
		if (zone_table[zrn].cmd[subcmd].command == 'V') {
			free(zone_table[zrn].cmd[subcmd].sarg1);
			free(zone_table[zrn].cmd[subcmd].sarg2);
		}
	}
	if (zone_table[zrn].cmd) {
		free(zone_table[zrn].cmd);
		zone_table[zrn].cmd = nullptr;
	}
	if (zone_table[zrn].typeA_count) {
		zone_table[zrn].typeA_count = 0;
		free(zone_table[zrn].typeA_list);
	}
	if (zone_table[zrn].typeB_count) {
		zone_table[zrn].typeB_count = 0;
		free(zone_table[zrn].typeB_list);
		free(zone_table[zrn].typeB_flag);
	}
	zone_table[zrn].name = "Зона для данжей";
	zone_table[zrn].reset_mode = 0;
	zone_table[zrn].top = zone_table[zrn].vnum * 100 + 99;
	zone_table[zrn].copy_from_zone = 0; //свободна для следующего данжа
}

void AddDungeonShopSeller(MobRnum mrn_from, MobRnum mrn_to) {
	MobVnum mvn_from = mob_index[mrn_from].vnum;
	auto &shop_list = MUD::Shops();
	for (const auto &shop : shop_list) {
		if (std::find(shop->mob_vnums().begin(), shop->mob_vnums().end(), mvn_from) != std::end(shop->mob_vnums())) {
			shop->add_mob_vnum(mob_index[mrn_to].vnum);
			mob_index[mrn_to].func = shop_ext;
		}
	}
}

void RemoveShopSeller(MobRnum mrn) {
	MobVnum mvn = mob_index[mrn].vnum;
	auto &shop_list = MUD::Shops();
	for (const auto &shop : shop_list) {
		auto it = std::find(shop->mob_vnums().begin(), shop->mob_vnums().end(), mvn);
		if (it != std::end(shop->mob_vnums())) {
			shop->remove_mob_vnum(it);
			mob_index[mrn].func = nullptr;
		}
	}
}

#define TRANS_MOB(arg) \
    if (mob_index[zone_table[zrn_to].cmd[subcmd].arg].vnum / 100 == zone_table[zrn_from].vnum) { \
        zone_table[zrn_to].cmd[subcmd].arg = GetMobRnum(mob_index[zone_table[zrn_from].cmd[subcmd].arg].vnum % 100 + zone_table[zrn_to].vnum * 100); }
#define TRANS_OBJ(arg) \
    if (obj_proto[zone_table[zrn_to].cmd[subcmd].arg]->get_vnum() / 100 == zone_table[zrn_from].vnum) { \
        zone_table[zrn_to].cmd[subcmd].arg = GetObjRnum(obj_proto[zone_table[zrn_from].cmd[subcmd].arg]->get_vnum() % 100 + zone_table[zrn_to].vnum * 100); \
    }
#define TRANS_ROOM(arg) \
    if (world[zone_table[zrn_to].cmd[subcmd].arg]->vnum / 100 == zone_table[zrn_from].vnum) { \
        zone_table[zrn_to].cmd[subcmd].arg = GetRoomRnum(world[zone_table[zrn_from].cmd[subcmd].arg]->vnum % 100 + zone_table[zrn_to].vnum * 100); }

void ZoneTransformCMD(ZoneRnum zrn_to, ZoneRnum zrn_from) {
	for (int subcmd = 0; zone_table[zrn_to].cmd[subcmd].command != 'S'; ++subcmd) {
		if (zone_table[zrn_to].cmd[subcmd].command == '*')
			continue;

//		log("CMD from %d %d %d %d %d %d",
//		zone_table[zrn_to].cmd[subcmd].command, zone_table[zrn_to].cmd[subcmd].if_flag,
//		zone_table[zrn_to].cmd[subcmd].arg1, zone_table[zrn_to].cmd[subcmd].arg2,
//		zone_table[zrn_to].cmd[subcmd].arg3, zone_table[zrn_to].cmd[subcmd].arg4);
		switch (zone_table[zrn_to].cmd[subcmd].command) {
			case 'M': TRANS_MOB(arg1)
				TRANS_ROOM(arg3)
				break;
			case 'F': TRANS_ROOM(arg1)
				TRANS_MOB(arg2)
				TRANS_MOB(arg3)
				break;
			case 'Q': TRANS_MOB(arg1)
				break;
			case 'O': TRANS_OBJ(arg1)
				TRANS_ROOM(arg3)
				break;
			case 'P': TRANS_OBJ(arg1)
				TRANS_OBJ(arg3)
				break;
			case 'G': [[fallthrough]];
			case 'E': TRANS_OBJ(arg1)
				break;
			case 'R': TRANS_ROOM(arg1)
				TRANS_OBJ(arg2)
				break;
			case 'D': TRANS_ROOM(arg1)
				break;
			case 'T':
				if (trig_index[zone_table[zrn_to].cmd[subcmd].arg2]->vnum / 100 == zone_table[zrn_from].vnum) { 
					zone_table[zrn_to].cmd[subcmd].arg2 = GetTriggerRnum(trig_index[zone_table[zrn_from].cmd[subcmd].arg2]->vnum % 100 + zone_table[zrn_to].vnum * 100); 
				}
				if (zone_table[zrn_to].cmd[subcmd].arg1 == WLD_TRIGGER) {
					TRANS_ROOM(arg3)
				}
				break;
			case 'V':
				if (zone_table[zrn_to].cmd[subcmd].arg1 == WLD_TRIGGER) {
					TRANS_ROOM(arg2)
				}
				break;
			default: break;
		}
	}
}

void SwapObjectDungeon(CharData *ch) {
	for (auto & i : ch->equipment) {
		if (i) {
			SwapOriginalObject(i);
		}
	}
	ObjData *obj_next = nullptr;
	ObjData *next_obj = nullptr;

	for (auto obj = ch->carrying; obj; obj = obj_next) {
		obj_next = obj->get_next_content();
		if (GET_OBJ_TYPE(obj) == EObjType::kContainer) {
			for (auto obj2 = obj->get_contains(); obj2; obj2 = next_obj) {
				next_obj = obj2->get_next_content();
				SwapOriginalObject(obj2);
			}
		}
		SwapOriginalObject(obj);
	}
}

void SwapOriginalObject(ObjData *obj) {
	if (obj->get_parent_rnum() > -1 && !obj->has_flag(EObjFlag::kRepopDecay)) {
		const auto obj_original = world_objects.create_from_prototype_by_rnum(obj->get_parent_rnum());
		int pos = -1;
		CharData *wearer = nullptr;
		ObjData *in_obj = nullptr;

		if (obj->get_worn_by()) {
			pos = obj->get_worn_on();
			wearer = obj->get_worn_by();
			UnequipChar(obj->get_worn_by(), pos, CharEquipFlags());
		}
		if (obj->get_in_obj()) {
			in_obj = obj->get_in_obj();
			RemoveObjFromObj(obj);
		}
		obj->swap(*obj_original.get());
		if (obj_original->has_flag(EObjFlag::kTicktimer)) {
			obj->set_extra_flag(EObjFlag::kTicktimer);
		}
		if (in_obj) {
			PlaceObjIntoObj(obj, in_obj);
		}
		if (wearer) {
			EquipObj(wearer, obj, pos, CharEquipFlags());
		}
		ExtractObjFromWorld(obj_original.get());
	}
}

} // namespace dungeons

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
