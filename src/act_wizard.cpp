/* ************************************************************************
*   File: act.wizard.cpp                                Part of Bylins    *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                       *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "administration/proxy.h"
#include "administration/ban.h"
#include "administration/punishments.h"
#include "gameplay/mechanics/celebrates.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/char_player.h"
#include "engine/entities/entities_constants.h"
#include "engine/db/world_characters.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/core/comm.h"
#include "engine/ui/cmd_god/do_shutdown.h"
#include "engine/core/config.h"
#include "gameplay/core/constants.h"
#include "engine/db/db.h"
#include "gameplay/mechanics/depot.h"
#include "engine/db/description.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/mem_queue.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "engine/ui/interpreter.h"
#include "gameplay/mechanics/liquid.h"
#include "utils/logger.h"
#include "gameplay/communication/mail.h"
#include "engine/ui/modify.h"
#include "engine/db/obj_prototypes.h"
#include "engine/olc/olc.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "gameplay/skills/skills.h"
#include "gameplay/magic/spells.h"
#include "engine/network/descriptor_data.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "utils/utils.h"
#include "engine/db/world_objects.h"
#include "engine/entities/zone.h"
#include "gameplay/classes/classes_constants.h"
#include "gameplay/magic/magic_rooms.h"
#include "utils/utils_time.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

using std::ifstream;
using std::fstream;

// external vars
extern FILE *player_fl;
extern int circle_restrict;
extern time_t zones_stat_date;
extern int check_dupes_host(DescriptorData *d, bool autocheck = false);

void medit_save_to_disk(int zone_num);

// for entities
void do_recall(CharData *ch, char *argument, int cmd, int subcmd);
void log_zone_count_reset();
// extern functions
void appear(CharData *ch);
int reserved_word(const char *name);
// local functions
void perform_immort_invis(CharData *ch, int level);
void DoSendMsgToChar(CharData *ch, char *argument, int, int);
RoomRnum FindTargetInRoom(CharData *ch, char *rawroomstr, int trig);
void DoAtRoom(CharData *ch, char *argument, int, int);
void DoGoto(CharData *ch, char *argument, int, int);
void DoTeleport(CharData *ch, char *argument, int, int);
void DoShutdown(CharData *ch, char *argument, int, int);
void StopSnooping(CharData *ch);
void DoSnoop(CharData *ch, char *argument, int, int);
void DoSwitch(CharData *ch, char *argument, int, int);
void DoReturn(CharData *ch, char *argument, int cmd, int subcmd);
void DoLoad(CharData *ch, char *argument, int cmd, int);
void DoVstat(CharData *ch, char *argument, int cmd, int);
void DoPurge(CharData *ch, char *argument, int, int);
void DoSyslog(CharData *ch, char *argument, int, int subcmd);
void DoAdvance(CharData *ch, char *argument, int, int);
void DoRestore(CharData *ch, char *argument, int, int subcmd);
void perform_immort_vis(CharData *ch);
void do_invis(CharData *ch, char *argument, int cmd, int subcmd);
void DoGlobalEcho(CharData *ch, char *argument, int, int);
void DoSetPoofMsg(CharData *ch, char *argument, int, int subcmd);
void DoWizlock(CharData *ch, char *argument, int, int);
void DoPageDateTime(CharData *ch, char *, int, int subcmd);
void DoPageLastLogins(CharData *ch, char *argument, int, int);
void do_force(CharData *ch, char *argument, int cmd, int subcmd);
void do_wiznet(CharData *ch, char *argument, int cmd, int subcmd);
void DoClearZone(CharData *ch, char *argument, int cmd, int);
void do_wizutil(CharData *ch, char *argument, int cmd, int subcmd);
void do_show(CharData *ch, char *argument, int cmd, int subcmd);
void do_liblist(CharData *ch, char *argument, int cmd, int subcmd);
//
void DoSendMsgToDemigods(CharData *ch, char *argument, int, int);
void DoUnfreeze(CharData *ch, char *, int, int);
void DoCheckZoneOccupation(CharData *ch, char *argument, int, int);
void DoDeleteObj(CharData *ch, char *argument, int, int);
void DoFindObjByRnum(CharData *ch, char *argument, int cmd, int subcmd);
void DoArenaRestore(CharData *ch, char *argument, int, int);
void DoShowZoneStat(CharData *ch, char *argument, int, int);
void DoPageClanOverstuff(CharData *ch, char *, int, int);
void DoSendTextToChar(CharData *ch, char *argument, int, int);
void generate_magic_enchant(ObjData *obj);

void log_zone_count_reset() {
	for (auto & i : zone_table) {
		sprintf(buf, "Zone: %d, count_reset: %d", i.vnum, i.count_reset);
		log("%s", buf);
	}
}

void DoSendTextToChar(CharData *ch, char *argument, int, int) {
	CharData *vict = nullptr;

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		SendMsgToChar("Кому и какой текст вы хотите отправить?\r\n", ch);
	} else if (!(vict = get_player_vis(ch, buf, EFind::kCharInWorld))) {
		SendMsgToChar("Такого персонажа нет в игре.\r\n", ch);
	} else if (vict->IsNpc())
		SendMsgToChar("Такого персонажа нет в игре.\r\n", ch);
	else {
		snprintf(buf1, kMaxStringLength, "%s\r\n", buf2);
		SendMsgToChar(buf1, vict);
	}
}

// показывает количество вещей (чтобы носить которые, нужно больше 8 ремортов) в хранах кланов
void DoPageClanOverstuff(CharData *ch, char *, int, int) {
	std::map<std::string, int> objects;
	for (const auto & clan : Clan::ClanList) {
		for (ObjData *chest = world[GetRoomRnum(clan->get_chest_room())]->contents; chest;
			 chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
					if (temp->get_auto_mort_req() > 8) {
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

// Функция для отправки текста богам
// При demigod = True, текст отправляется и демигодам тоже
void SendMsgToGods(char *text, bool demigod) {
	DescriptorData *d;
	for (d = descriptor_list; d; d = d->next) {
		// Чар должен быть в игре
		if (STATE(d) == CON_PLAYING) {
			// Чар должен быть имморталом
			// Либо же демигодом (при demigod = true)
			if ((GetRealLevel(d->character) >= kLvlGod) ||
				(GET_GOD_FLAG(d->character, EGf::kDemigod) && demigod)) {
				SendMsgToChar(text, d->character.get());
			}
		}
	}
}

extern const char *deaf_social;

extern bool print_object_location(int num, const ObjData *obj, CharData *ch);

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
			SendMsgToChar(ch, "Найден предмет с ошибкой в реальном количестве %s #%d sum = %ld \r\n", GET_OBJ_PNAME(*it, 0).c_str(), (*it)->get_vnum(), sum);
			for (auto object : objs) {
				print_object_location(num++, object, ch);
			}
			for (auto object : objs_orig) {
				print_object_location(num++, object, ch);
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
		print_object_location(num++, object, ch);
	}
}

void DoDeleteObj(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int vnum;
	one_argument(argument, buf);
	int num = 0;
	if (!*buf || !a_isdigit(*buf)) {
		SendMsgToChar("Usage: delete <number>\r\n", ch);
		return;
	}
	if ((vnum = atoi(buf)) < 0) {
		SendMsgToChar("Указан неверный VNUM объекта !\r\n", ch);
		return;
	}

	world_objects.foreach_with_vnum(vnum, [&num](const ObjData::shared_ptr &k) {
		k->set_timer(0);
		++num;
	});
//	кланхран вещи в игре их не надо чистить, удалены выше.
//	num += Clan::delete_obj(vnum);
	num += Depot::delete_obj(vnum);
	num += Parcel::delete_obj(vnum);
	sprintf(buf2, "Удалено всего предметов: %d, смотрим ренту.\r\n", num);
	SendMsgToChar(buf2, ch);
	num = 0;
	for (std::size_t pt_num = 0; pt_num< player_table.size(); pt_num++) {
		bool need_save = false;
	// рента
		if (player_table[pt_num].timer) {
			for (auto i = player_table[pt_num].timer->time.begin(),
					 iend = player_table[pt_num].timer->time.end(); i != iend; ++i) {
				if (i->vnum == vnum && i->timer > 0) {
					num++;
					sprintf(buf2, "Player %s : item [%d] deleted\r\n", player_table[pt_num].name(), i->vnum);;
					SendMsgToChar(buf2, ch);
					i->timer = -1;
					int rnum = GetObjRnum(i->vnum);
					if (rnum >= 0) {
						obj_proto.dec_stored(rnum);
					}
					need_save = true;
				}
			}
		}
		if (need_save) {
			if (!Crash_write_timer(pt_num)) {
				sprintf(buf, "SYSERROR: [TO] Error writing timer file for %s", player_table[pt_num].name());
				SendMsgToChar(buf2, ch);
			}
		}
	}
	sprintf(buf2, "Удалено еще предметов: %d.\r\n", num);
	SendMsgToChar(buf2, ch);
}

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
	if (tmp1 >= 0 && !*arg2) {
		PrintZoneStat(ch, tmp1, tmp1, sort);
		return;
	}
	if (tmp1 > 0 && tmp2 > 0) {
		PrintZoneStat(ch, tmp1, tmp2, sort);
		return;
	}
	SendMsgToChar(ch, "Зоныстат формат: 'все' или диапазон через пробел, -s в конце для сортировки. 'очистить' новая таблица\r\n");
}

void CheckCharactersInZone(ZoneRnum zone_nr, CharData *ch) {
	DescriptorData *i;
	int rnum_start, rnum_stop;
	bool found = false;

	if (room_spells::IsZoneRoomAffected(zone_nr, ESpell::kRuneLabel)) {
		SendMsgToChar("В зоне имеется рунная метка.\r\n", ch);
	}

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING)
			continue;
		if (i->character->in_room == kNowhere)
			continue;
		if (GetRealLevel(i->character) >= kLvlImmortal)
			continue;
		if (world[i->character->in_room]->zone_rn != zone_nr)
			continue;
		sprintf(buf2,
				"Проверка по дискрипторам: В зоне (vnum: %d клетка: %d) находится персонаж: %s.\r\n",
				zone_table[zone_nr].vnum,
				GET_ROOM_VNUM(i->character->in_room),
				GET_NAME(i->character));
		SendMsgToChar(buf2, ch);
		found = true;
	}
	if (found)
		return;
	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!GetZoneRooms(zone_nr, &rnum_start, &rnum_stop)) {
		sprintf(buf2, "Нет комнат в зоне %d.", static_cast<int>(zone_table[zone_nr].vnum));
		SendMsgToChar(buf2, ch);
		return;    // в зоне нет комнат :)
	}

	for (; rnum_start <= rnum_stop; rnum_start++) {
		// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		{
			for (const auto c : world[rnum_start]->people) {
				if (!c->IsNpc() && (GetRealLevel(c) < kLvlImmortal)) {
					sprintf(buf2,
							"Проверка по списку чаров (с учетом linkdrop): в зоне vnum: %d клетка: %d находится персонаж: %s.\r\n",
							zone_table[zone_nr].vnum,
							GET_ROOM_VNUM(c->in_room),
							GET_NAME(c));
					SendMsgToChar(buf2, ch);
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

		sprintf(buf2,
				"В прокси руме сидит игрок %s находящийся в зоне vnum: %d клетка: %d\r\n",
				GET_NAME(c),
				zone_table[zone_nr].vnum,
				GET_ROOM_VNUM(c->in_room));
		SendMsgToChar(buf2, ch);
		found = true;
	}

	if (!found) {
		sprintf(buf2, "В зоне %d даже мышь не пробегала.\r\n", zone_table[zone_nr].vnum);
		SendMsgToChar(buf2, ch);
	}
}

void DoCheckZoneOccupation(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int number;
	ZoneRnum zrn;
	one_argument(argument, buf);
	bool is_found = false;
	if (!*buf || !a_isdigit(*buf)) {
		SendMsgToChar("Usage: занятость внумзоны\r\n", ch);
		return;
	}

	if ((number = atoi(buf)) < 0) {
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

void DoSendMsgToChar(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	half_chop(argument, arg, buf);

	if (!*arg) {
		SendMsgToChar("Послать что и кому (не путать с куда и кого :)\r\n", ch);
		return;
	}
	if (!(vict = get_player_vis(ch, arg, EFind::kCharInWorld))) {
		SendMsgToChar(NOPERSON, ch);
		return;
	}
	SendMsgToChar(buf, vict);
	SendMsgToChar("\r\n", vict);
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar("Послано.\r\n", ch);
	else {
		snprintf(buf2, kMaxStringLength, "Вы послали '%s' %s.\r\n", buf, GET_PAD(vict, 2));
		SendMsgToChar(buf2, ch);
	}
}

// Take a string, and return a rnum. Used for goto, at, etc.  -je 4/6/93
RoomRnum FindTargetInRoom(CharData *ch, char *rawroomstr, int trig) {
	RoomVnum tmp;
	RoomRnum location;
	CharData *target_mob;
	ObjData *target_obj;
	char roomstr[kMaxInputLength];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		SendMsgToChar("Укажите номер или название комнаты.\r\n", ch);
		return (kNowhere);
	}
	if (a_isdigit(*roomstr) && !strchr(roomstr, '.')) {
		tmp = atoi(roomstr);
		if ((location = GetRoomRnum(tmp)) == kNowhere) {
			SendMsgToChar("Нет комнаты с таким номером.\r\n", ch);
			return (kNowhere);
		}
	} else if ((target_mob = get_char_vis(ch, roomstr, EFind::kCharInWorld)) != nullptr) {
		location = target_mob->in_room;
	} else if ((target_obj = get_obj_vis(ch, roomstr)) != nullptr) {
		if (target_obj->get_in_room() != kNowhere) {
			location = target_obj->get_in_room();
		} else {
			SendMsgToChar("Этот объект вам недоступен.\r\n", ch);
			return (kNowhere);
		}
	} else {
		SendMsgToChar("В округе нет похожего предмета или создания.\r\n", ch);
		return (kNowhere);
	}

	// a location has been found -- if you're < GRGOD, check restrictions.
	if (!IS_GRGOD(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		if (ROOM_FLAGGED(location, ERoomFlag::kGodsRoom) && GetRealLevel(ch) < kLvlGreatGod) {
			SendMsgToChar("Вы не столь божественны, чтобы получить доступ в эту комнату!\r\n", ch);
			return (kNowhere);
		}
		if (ROOM_FLAGGED(location, ERoomFlag::kNoTeleportIn) && trig != 1) {
			SendMsgToChar("В комнату не телепортировать!\r\n", ch);
			return (kNowhere);
		}
		if (!Clan::MayEnter(ch, location, kHousePortal)) {
			SendMsgToChar("Частная собственность - посторонним в ней делать нечего!\r\n", ch);
			return (kNowhere);
		}
	}
	return (location);
}

void DoAtRoom(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char command[kMaxInputLength];
	RoomRnum location, original_loc;

	half_chop(argument, buf, command);
	if (!*buf) {
		SendMsgToChar("Необходимо указать номер или название комнаты.\r\n", ch);
		return;
	}

	if (!*command) {
		SendMsgToChar("Что вы собираетесь там делать?\r\n", ch);
		return;
	}

	if ((location = FindTargetInRoom(ch, buf, 0)) == kNowhere)
		return;

	// a location has been found.
	original_loc = ch->in_room;
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	command_interpreter(ch, command);

	// check if the char is still there
	if (ch->in_room == location) {
		RemoveCharFromRoom(ch);
		PlaceCharToRoom(ch, original_loc);
	}
	ch->dismount();
}

void DoUnfreeze(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	/*Формат файл unfreeze.lst
	Первая строка email
	Вторая строка причина по которой разфриз
	Все остальные строки полные имена чаров*/
	//char email[50], reason[50];
	Player t_vict;
	CharData *vict;
	char *reason_c; // для функции set_punish, она не умеет принимать тип string :(
	std::string email;
	std::string reason;
	std::string name_buffer;
	ifstream unfreeze_list;
	unfreeze_list.open("../lib/misc/unfreeze.lst", fstream::in);
	if (!unfreeze_list) {
		SendMsgToChar("Файл unfreeze.lst отсутствует!\r\n", ch);
		return;
	}
	unfreeze_list >> email;
	unfreeze_list >> reason;
	sprintf(buf, "Начинаем масс.разфриз\r\nEmail:%s\r\nПричина:%s\r\n", email.c_str(), reason.c_str());
	SendMsgToChar(buf, ch);
	reason_c = new char[reason.length() + 1];
	strcpy(reason_c, reason.c_str());

	while (!unfreeze_list.eof()) {
		unfreeze_list >> name_buffer;
		if (LoadPlayerCharacter(name_buffer.c_str(), &t_vict, ELoadCharFlags::kFindId) < 0) {
			sprintf(buf, "Чара с именем %s не существует !\r\n", name_buffer.c_str());
			SendMsgToChar(buf, ch);
			continue;
		}
		vict = &t_vict;
		if (GET_EMAIL(vict) != email) {
			sprintf(buf, "У чара %s другой емайл.\r\n", name_buffer.c_str());
			SendMsgToChar(buf, ch);
			continue;
		}
		punishments::set_punish(ch, vict, SCMD_FREEZE, reason_c, 0);
		vict->save_char();
		sprintf(buf, "Чар %s разморожен.\r\n", name_buffer.c_str());
		SendMsgToChar(buf, ch);
	}

	delete[] reason_c;
	unfreeze_list.close();

}

void DoGoto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum location;

	if ((location = FindTargetInRoom(ch, argument, 0)) == kNowhere)
		return;

	if (POOFOUT(ch))
		sprintf(buf, "$n %s", POOFOUT(ch));
	else
		strcpy(buf, "$n растворил$u в клубах дыма.");

	act(buf, true, ch, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	ch->dismount();

	if (POOFIN(ch))
		sprintf(buf, "$n %s", POOFIN(ch));
	else
		strcpy(buf, "$n возник$q посреди комнаты.");
	act(buf, true, ch, nullptr, nullptr, kToRoom);
	look_at_room(ch, 0);
}

void DoTeleport(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	RoomRnum target;

	two_arguments(argument, buf, buf2);

	if (!*buf)
		SendMsgToChar("Кого вы хотите переместить?\r\n", ch);
	else if (!(victim = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else if (victim == ch)
		SendMsgToChar("Используйте 'прыжок' для собственного перемещения.\r\n", ch);
	else if (GetRealLevel(victim) >= GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo))
		SendMsgToChar("Попробуйте придумать что-то другое.\r\n", ch);
	else if (!*buf2)
		act("Куда вы хотите $S переместить?", false, ch, nullptr, victim, kToChar);
	else if ((target = FindTargetInRoom(ch, buf2, 0)) != kNowhere) {
		SendMsgToChar(OK, ch);
		act("$n растворил$u в клубах дыма.", false, victim, nullptr, nullptr, kToRoom);
		RemoveCharFromRoom(victim);
		PlaceCharToRoom(victim, target);
		victim->dismount();
		act("$n появил$u, окутанн$w розовым туманом.",
			false, victim, nullptr, nullptr, kToRoom);
		act("$n переместил$g вас!", false, ch, nullptr, (char *) victim, kToVict);
		look_at_room(victim, 0);
	}
}

void DoShutdown(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	commands::Shutdown command(ch, argument, shutdown_parameters);
	if (command.parse_arguments()) {
		command.execute();
	}
}

void StopSnooping(CharData *ch) {
	if (!ch->desc->snooping)
		SendMsgToChar("Вы не подслушиваете.\r\n", ch);
	else {
		SendMsgToChar("Вы прекратили подслушивать.\r\n", ch);
		ch->desc->snooping->snoop_by = nullptr;
		ch->desc->snooping = nullptr;
	}
}

void DoSnoop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim, *tch;

	if (!ch->desc)
		return;

	argument = one_argument(argument, arg);

	if (!*arg)
		StopSnooping(ch);
	else if (!(victim = get_player_vis(ch, arg, EFind::kCharInWorld)))
		SendMsgToChar("Нет такого создания в игре.\r\n", ch);
	else if (!victim->desc)
		act("Вы не можете $S подслушать - он$G потерял$G связь..\r\n",
			false, ch, nullptr, victim, kToChar);
	else if (victim == ch)
		StopSnooping(ch);
	else if (victim->desc->snooping == ch->desc)
		SendMsgToChar("Вы уже подслушиваете.\r\n", ch);
	else if (victim->desc->snoop_by && victim->desc->snoop_by != ch->desc)
		SendMsgToChar("Дык его уже кто-то из богов подслушивает.\r\n", ch);
	else {
		if (victim->desc->original)
			tch = victim->desc->original.get();
		else
			tch = victim;

		const int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		const int victim_level = tch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(tch);

		if (victim_level >= god_level) {
			SendMsgToChar("Вы не можете.\r\n", ch);
			return;
		}
		SendMsgToChar(OK, ch);

		ch->desc->snoop_with_map = false;
		if (god_level >= kLvlImplementator && argument && *argument) {
			skip_spaces(&argument);
			if (isname(argument, "map") || isname(argument, "карта")) {
				ch->desc->snoop_with_map = true;
			}
		}

		if (ch->desc->snooping)
			ch->desc->snooping->snoop_by = nullptr;

		ch->desc->snooping = victim->desc;
		victim->desc->snoop_by = ch->desc;
	}
}

void DoSwitch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);

	if (ch->desc->original) {
		SendMsgToChar("Вы уже в чьем-то теле.\r\n", ch);
	} else if (!*arg) {
		SendMsgToChar("Стать кем?\r\n", ch);
	} else {
		const auto visible_character = get_char_vis(ch, arg, EFind::kCharInWorld);
		if (!visible_character) {
			SendMsgToChar("Нет такого создания.\r\n", ch);
		} else if (ch == visible_character) {
			SendMsgToChar("Вы и так им являетесь.\r\n", ch);
		} else if (visible_character->desc) {
			SendMsgToChar("Это тело уже под контролем.\r\n", ch);
		} else if (!IS_IMPL(ch)
			&& !visible_character->IsNpc()) {
			SendMsgToChar("Вы не столь могущественны, чтобы контроолировать тело игрока.\r\n", ch);
		} else if (GetRealLevel(ch) < kLvlGreatGod
			&& ROOM_FLAGGED(visible_character->in_room, ERoomFlag::kGodsRoom)) {
			SendMsgToChar("Вы не можете находиться в той комнате.\r\n", ch);
		} else if (!IS_GRGOD(ch)
			&& !Clan::MayEnter(ch, visible_character->in_room, kHousePortal)) {
			SendMsgToChar("Вы не сможете проникнуть на частную территорию.\r\n", ch);
		} else {
			const auto victim = character_list.get_character_by_address(visible_character);
			const auto me = character_list.get_character_by_address(ch);
			if (!victim || !me) {
				SendMsgToChar("Something went wrong. Report this bug to developers\r\n", ch);
				return;
			}

			SendMsgToChar(OK, ch);

			ch->desc->character = victim;
			ch->desc->original = me;

			victim->desc = ch->desc;
			ch->desc = nullptr;
		}
	}
}

void DoReturn(CharData *ch, char *argument, int cmd, int subcmd) {
	if (ch->desc && ch->desc->original) {
		SendMsgToChar("Вы вернулись в свое тело.\r\n", ch);

		/*
		 * If someone switched into your original body, disconnect them.
		 *   - JE 2/22/95
		 *
		 * Zmey: here we put someone switched in our body to disconnect state
		 * but we must also NULL his pointer to our character, otherwise
		 * close_socket() will damage our character's pointer to our descriptor
		 * (which is assigned below in this function). 12/17/99
		 */
		if (ch->desc->original->desc) {
			ch->desc->original->desc->character = nullptr;
			STATE(ch->desc->original->desc) = CON_DISCONNECT;
		}
		ch->desc->character = ch->desc->original;
		ch->desc->original = nullptr;

		ch->desc->character->desc = ch->desc;
		ch->desc = nullptr;
	} else {
		do_recall(ch, argument, cmd, subcmd);
	}
}

// отправка сообщения вообще всем
void send_to_all(char *buffer) {
	DescriptorData *pt;
	for (pt = descriptor_list; pt; pt = pt->next) {
		if (STATE(pt) == CON_PLAYING && pt->character) {
			SendMsgToChar(buffer, pt->character.get());
		}
	}
}

void DoVstat(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	CharData *mob;
	MobVnum number;    // or ObjVnum ...
	MobRnum r_num;        // or ObjRnum ...

	two_arguments(argument, buf, buf2);
	int first = atoi(buf2) / 100;

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <= 0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (!*buf || !*buf2 || !a_isdigit(*buf2)) {
		SendMsgToChar("Usage: vstat { obj | mob } <number>\r\n", ch);
		return;
	}


	if (!IS_IMMORTAL(ch) && GET_OLC_ZONE(ch) != first) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}

	if ((number = atoi(buf2)) < 0) {
		SendMsgToChar("Отрицательный номер? Оригинально!\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf, "mob")) {
		if ((r_num = GetMobRnum(number)) < 0) {
			SendMsgToChar("Обратитесь в Арктику - там ОН живет.\r\n", ch);
			return;
		}
		mob = ReadMobile(r_num, kReal);
		PlaceCharToRoom(mob, 1);
		do_stat_character(ch, mob, 1);
		ExtractCharFromWorld(mob, false);
	} else if (utils::IsAbbr(buf, "obj")) {
		if ((r_num = GetObjRnum(number)) < 0) {
			SendMsgToChar("Этот предмет явно перенесли в РМУД.\r\n", ch);
			return;
		}

		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		do_stat_object(ch, obj.get(), 1);
		ExtractObjFromWorld(obj.get());
	} else
		SendMsgToChar("Тут должно быть что-то типа 'obj' или 'mob'.\r\n", ch);
}

// clean a room of all mobiles and objects
void DoPurge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	ObjData *obj, *next_o;

	one_argument(argument, buf);

	if (*buf) {        // argument supplied. destroy single object or char
		if ((vict = get_char_vis(ch, buf, EFind::kCharInRoom)) != nullptr) {
			if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Да я вас за это...\r\n", ch);
				return;
			}
			act("$n обратил$g в прах $N3.", false, ch, nullptr, vict, kToNotVict);
			if (!vict->IsNpc()) {
				sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, CMP, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				if (vict->desc) {
					STATE(vict->desc) = CON_CLOSE;
					vict->desc->character = nullptr;
					vict->desc = nullptr;
				}
			}
			ExtractCharFromWorld(vict, false);
		} else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room]->contents)) != nullptr) {
			act("$n просто разметал$g $o3 на молекулы.", false, ch, obj, nullptr, kToRoom);
			ExtractObjFromWorld(obj);
		} else {
			SendMsgToChar("Ничего похожего с таким именем нет.\r\n", ch);
			return;
		}
		SendMsgToChar(OK, ch);
	} else        // no argument. clean out the room
	{
		act("$n произнес$q СЛОВО... вас окружило пламя!", false, ch, nullptr, nullptr, kToRoom);
		SendMsgToRoom("Мир стал немного чище.\r\n", ch->in_room, false);
		for (obj = world[ch->in_room]->contents; obj; obj = next_o) { //сначала шмотки, иначе потетеряешь весь стаф с случайных чармисов
			next_o = obj->get_next_content();
			ExtractObjFromWorld(obj);
		}
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (vict->IsNpc()) {
				if (vict->followers
					|| vict->has_master()) {
					die_follower(vict);
				}
				if (!vict->purged()) {
					ExtractCharFromWorld(vict, false);
				}
			}
		}
	}
}

const char *logtypes[] =
	{
		"нет", "начальный", "краткий", "нормальный", "полный", "\n"
	};

// subcmd - канал
void DoSyslog(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int tp;

	if (subcmd < 0 || subcmd > LAST_LOG) {
		return;
	}

	tp = GET_LOGS(ch)[subcmd];
	if (tp > 4)
		tp = 4;
	if (tp < 0)
		tp = 0;

	one_argument(argument, arg);

	if (*arg) {
		if (GetRealLevel(ch) == kLvlImmortal)
			logtypes[2] = "\n";
		else
			logtypes[2] = "краткий";
		if (GetRealLevel(ch) == kLvlGod)
			logtypes[4] = "\n";
		else
			logtypes[4] = "полный";
		if ((tp = search_block(arg, logtypes, false)) == -1) {
			if (GetRealLevel(ch) == kLvlImmortal)
				SendMsgToChar("Формат: syslog { нет | начальный }\r\n", ch);
			else if (GetRealLevel(ch) == kLvlGod)
				SendMsgToChar("Формат: syslog { нет | начальный | краткий | нормальный }\r\n", ch);
			else
				SendMsgToChar
					("Формат: syslog { нет | начальный | краткий | нормальный | полный }\r\n", ch);
			return;
		}
		GET_LOGS(ch)[subcmd] = tp;
	}
	sprintf(buf,
			"Тип вашего лога (%s) сейчас %s.\r\n",
			runtime_config.logs(static_cast<EOutputStream>(subcmd)).title().c_str(),
			logtypes[tp]);
	SendMsgToChar(buf, ch);
}

void DoRestore(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;

	one_argument(argument, buf);
	if (!*buf)
		SendMsgToChar("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else {
		// имм с привилегией arena может ресторить только чаров, находящихся с ним на этой же арене
		// плюс исключается ситуация, когда они в одной зоне, но чар не в клетке арены
		if (privilege::CheckFlag(ch, privilege::kArenaMaster)) {
			if (!ROOM_FLAGGED(vict->in_room, ERoomFlag::kArena) || world[ch->in_room]->zone_rn != world[vict->in_room]->zone_rn) {
				SendMsgToChar("Не положено...\r\n", ch);
				return;
			}
		}

		vict->set_hit(vict->get_real_max_hit());
		vict->set_move(vict->get_real_max_move());
		if (IS_MANA_CASTER(vict)) {
			vict->mem_queue.stored = GET_MAX_MANA(vict);
		} else {
			vict->mem_queue.stored = vict->mem_queue.total;
		}
		if (vict->GetSkill(ESkill::kWarcry) > 0) {
			struct TimedSkill wctimed;
			wctimed.skill = ESkill::kWarcry;
			wctimed.time = 0;
			ImposeTimedSkill(vict, &wctimed);
		}
		if (IS_GRGOD(ch) && IS_IMMORTAL(vict)) {
			vict->set_str(25);
			vict->set_int(25);
			vict->set_wis(25);
			vict->set_dex(25);
			vict->set_con(25);
			vict->set_cha(25);
		}
		update_pos(vict);
		RemoveAffectFromChar(vict, ESpell::kDrunked);
		GET_DRUNK_STATE(vict) = GET_COND(vict, DRUNK) = 0;
		RemoveAffectFromChar(vict, ESpell::kAbstinent);

		//сброс таймеров скиллов и фитов
		while (vict->timed)
			ExpireTimedSkill(vict, vict->timed);
		while (vict->timed_feat)
			ExpireTimedFeat(vict, vict->timed_feat);

		if (subcmd == SCMD_RESTORE_GOD) {
			SendMsgToChar(OK, ch);
			act("Вы были полностью восстановлены $N4!",
				false, vict, nullptr, ch, kToChar);
		}
		affect_total(vict);
	}
}

void DoGlobalEcho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *pt;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument) {
		SendMsgToChar("Это, пожалуй, ошибка...\r\n", ch);
	} else {
		sprintf(buf, "%s\r\n", argument);
		for (pt = descriptor_list; pt; pt = pt->next) {
			if (STATE(pt) == CON_PLAYING
				&& pt->character
				&& pt->character.get() != ch) {
				SendMsgToChar(buf, pt->character.get());
			}
		}

		if (ch->IsFlagged(EPrf::kNoRepeat)) {
			SendMsgToChar(OK, ch);
		} else {
			SendMsgToChar(buf, ch);
		}
	}
}

void DoSetPoofMsg(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char **msg;

	switch (subcmd) {
		case SCMD_POOFIN: msg = &(POOFIN(ch));
			break;
		case SCMD_POOFOUT: msg = &(POOFOUT(ch));
			break;
		default: return;
	}

	skip_spaces(&argument);

	if (*msg)
		free(*msg);

	if (!*argument)
		*msg = nullptr;
	else
		*msg = str_dup(argument);

	SendMsgToChar(OK, ch);
}

void DoWizlock(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int value;
	const char *when;

	one_argument(argument, arg);
	if (*arg) {
		value = atoi(arg);
		if (value > kLvlImplementator)
			value = kLvlImplementator; // 34е всегда должны иметь возможность зайти
		if (value < 0 || (value > GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo))) {
			SendMsgToChar("Неверное значение для wizlock.\r\n", ch);
			return;
		}
		circle_restrict = value;
		when = "теперь";
	} else
		when = "в настоящее время";

	switch (circle_restrict) {
		case 0: sprintf(buf, "Игра %s полностью открыта.\r\n", when);
			break;
		case 1: sprintf(buf, "Игра %s закрыта для новых игроков.\r\n", when);
			break;
		default:
			sprintf(buf, "Только игроки %d %s и выше могут %s войти в игру.\r\n",
					circle_restrict, GetDeclensionInNumber(circle_restrict, EWhat::kLvl), when);
			break;
	}
	SendMsgToChar(buf, ch);
}

extern void PrintUptime(std::ostringstream &out);

void DoPageDateTime(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	time_t mytime;
	std::ostringstream out;

	if (subcmd == SCMD_DATE) {
		mytime = time(nullptr);
		out << "Текущее время сервера: " << asctime(localtime(&mytime)) << "\r\n";
	} else {
		mytime = shutdown_parameters.get_boot_time();
		out << " Up since: " << asctime(localtime(&mytime));
		out.seekp(-1, std::ios_base::end); // Удаляем \0 из конца строки.
		out << " ";
		PrintUptime(out);
	}

	SendMsgToChar(out.str(), ch);
}

void DoPageLastLogins(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Кого вы хотите найти?\r\n", ch);
		return;
	}

	Player t_chdata;
	Player *chdata = &t_chdata;
	if (LoadPlayerCharacter(arg, chdata, ELoadCharFlags::kFindId) < 0) {
		SendMsgToChar("Нет такого игрока.\r\n", ch);
		return;
	}
	if (GetRealLevel(chdata) > GetRealLevel(ch) && !IS_IMPL(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("Вы не столь уж и божественны для этого.\r\n", ch);
	} else {
		time_t tmp_time = LAST_LOGON(chdata);
		sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
				GET_UID(chdata), GetRealLevel(chdata),
				MUD::Class(chdata->GetClass()).GetAbbr().c_str(), GET_NAME(chdata),
				GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown", ctime(&tmp_time));
		SendMsgToChar(buf, ch);
	}
}

void DoSendMsgToDemigods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	// убираем пробелы
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("Что Вы хотите сообщить ?\r\n", ch);
		return;
	}
	sprintf(buf1, "&c%s демигодам: '%s'&n\r\n", GET_NAME(ch), argument);

	// проходим по всем чарам и отправляем мессагу
	for (d = descriptor_list; d; d = d->next) {
		// Проверяем, в игре ли чар
		if (STATE(d) == CON_PLAYING) {
			// Если в игре, то проверяем, демигод ли чар
			// Иммы 34-левела тоже могут смотреть канал
			if ((GET_GOD_FLAG(d->character, EGf::kDemigod)) || (GetRealLevel(d->character) == kLvlImplementator)) {
				// Проверяем пишет ли чар или отправляет письмо
				// А так же на реж сдемигод
				if ((!d->character->IsFlagged(EPlrFlag::kWriting)) &&
					(!d->character->IsFlagged(EPlrFlag::kMailing)) &&
					(!d->character->IsFlagged(EPrf::kDemigodChat))) {
					d->character->remember_add(buf1, Remember::ALL);
					SendMsgToChar(buf1, d->character.get());
				}
			}
		}
	}
}

void DoClearZone(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	UniqueList<ZoneRnum> zone_repop_list;
	RoomRnum rrn_start = 0;
	ZoneRnum zrn;

	one_argument(argument, arg);
	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false))) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*arg) {
		SendMsgToChar("Укажите зону.\r\n", ch);
		return;
	}
	if (*arg == '.') {
		zrn = world[ch->in_room]->zone_rn;
	} else {
		zrn = GetZoneRnum(atoi(arg));
	}
	if (zrn > 0 || *arg == '.' || *arg == '1') {
		utils::CExecutionTimer timer;

		sprintf(buf, "Очищаю и перегружаю зону #%d: %s\r\n", zone_table[zrn].vnum, zone_table[zrn].name.c_str());
		SendMsgToChar(buf, ch);
		rrn_start = zone_table[zrn].RnumRoomsLocation.first;
		for (RoomVnum rvn = 0; rvn <= 99; rvn++) {
			auto &room = world[rrn_start + rvn];

			dungeons::ClearRoom(room);
		}
		zone_repop_list.push_back(zrn);
		DecayObjectsOnRepop(zone_repop_list);
		ResetZone(zrn);
		sprintf(buf, "(GC) %s clear and reset zone %d (%s), delta %f", GET_NAME(ch), zrn, zone_table[zrn].name.c_str(), timer.delta().count());
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s clear and reset zone %d (%s)", GET_NAME(ch), zrn, zone_table[zrn].name.c_str());
	} else {
		SendMsgToChar("Нет такой зоны.\r\n", ch);
	}
}

///////////////////////////////////////////////////////////////////////////////
namespace SpellUsage {
bool is_active = false;
std::map<ECharClass, SpellCountType> usage;
const char *SPELL_STAT_FILE = LIB_STAT"spellstat.txt";
time_t start;
}

void SpellUsage::clear() {
	for (auto & it : usage) {
		it.second.clear();
	}
	usage.clear();
	start = time(nullptr);
}

std::string StatToPrint() {
	std::stringstream out;
	time_t now = time(nullptr);
	char *end_time = str_dup(rustime(localtime(&now)));
	out << rustime(localtime(&SpellUsage::start)) << " - " << end_time << "\n";
	for (auto & it : SpellUsage::usage) {
		out << std::setw(35) << MUD::Class(it.first).GetName() << "\r\n";
		for (auto & itt : it.second) {
			out << std::setw(25) << MUD::Spell(itt.first).GetName() << " : " << itt.second << "\r\n";
		}
	}
	return out.str();
}

void SpellUsage::save() {
	if (!is_active)
		return;

	std::ofstream file(SPELL_STAT_FILE, std::ios_base::app | std::ios_base::out);

	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", SPELL_STAT_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	file << StatToPrint();
	file.close();
}

void SpellUsage::AddSpellStat(ECharClass char_class, ESpell spell_id) {
	if (!is_active) {
		return;
	}
	if (MUD::Classes().IsUnavailable(char_class) || spell_id > ESpell::kLast) {
		return;
	}
	++usage[char_class][spell_id];
}

void DoPageSpellStat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("заклстат [стоп|старт|очистить|показать|сохранить]\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "старт")) {
		SpellUsage::is_active = true;
		SpellUsage::start = time(nullptr);
		SendMsgToChar("Сбор включен.\r\n", ch);
		return;
	}

	if (!SpellUsage::is_active) {
		SendMsgToChar("Сбор выключен. Включите сбор 'заклстат старт'.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "стоп")) {
		SpellUsage::clear();
		SpellUsage::is_active = false;
		SendMsgToChar("Сбор выключен.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "показать")) {
		page_string(ch->desc, StatToPrint());
		return;
	}

	if (!str_cmp(argument, "очистить")) {
		SpellUsage::clear();
		return;
	}

	if (!str_cmp(argument, "сохранить")) {
		SpellUsage::save();
		return;
	}

	SendMsgToChar("заклстат: неизвестный аргумент\r\n", ch);
}

void DoSanitize(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Запущена процедура сбора мусора после праздника...\r\n", ch);
	celebrates::Sanitize();
}

void DoLoadstat(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	std::ifstream istream(LOAD_LOG_FOLDER LOAD_LOG_FILE, std::ifstream::in);
	int length;

	if (!istream.is_open()) {
		SendMsgToChar("Can't open file", ch);
		log("ERROR: Can't open file %s", LOAD_LOG_FOLDER LOAD_LOG_FILE);
		return;
	}

	istream.seekg(0, std::ifstream::end);
	length = istream.tellg();
	istream.seekg(0, std::ifstream::beg);
	istream.read(buf, std::min(length, kMaxStringLength - 1));
	buf[istream.gcount()] = '\0';
	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
