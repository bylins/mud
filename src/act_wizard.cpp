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

#include "act_wizard.h"

#include "action_targeting.h"
#include "cmd_god/ban.h"
#include "game_mechanics/birthplaces.h"
#include "game_mechanics/celebrates.h"
#include "game_mechanics/guilds.h"
#include "utils/utils_char_obj.inl"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/player_races.h"
#include "entities/entities_constants.h"
#include "entities/world_characters.h"
#include "cmd_god/stat.h"
#include "cmd/follow.h"
#include "comm.h"
#include "cmd_god/shutdown.h"
#include "conf.h"
#include "config.h"
#include "constants.h"
#include "corpse.h"
#include "db.h"
#include "depot.h"
#include "description.h"
#include "dg_script/dg_scripts.h"
#include "dg_script/dg_event.h"
#include "game_economics/ext_money.h"
#include "game_fight/fight.h"
#include "game_fight/pk.h"
#include "utils/file_crc.h"
#include "genchar.h"
#include "structs/global_objects.h"
#include "game_classes/classes.h"
#include "game_mechanics/glory.h"
#include "game_mechanics/glory_const.h"
#include "game_mechanics/glory_misc.h"
#include "game_mechanics/mem_queue.h"
#include "handler.h"
#include "heartbeat.h"
#include "house.h"
#include "game_crafts/im.h"
#include "interpreter.h"
#include "liquid.h"
#include "utils/logger.h"
#include "communication/mail.h"
#include "statistics/mob_stat.h"
#include "modify.h"
#include "administration/names.h"
#include "noob.h"
#include "entities/obj_data.h"
#include "obj_prototypes.h"
#include "olc/olc.h"
#include "communication/parcel.h"
#include "administration/password.h"
#include "administration/privilege.h"
#include "utils/pugixml/pugixml.h"
#include "entities/room_data.h"
#include "color.h"
#include "game_mechanics/sets_drop.h"
#include "game_economics/shop_ext.h"
#include "game_skills/skills.h"
#include "game_magic/spells.h"
#include "structs/descriptor_data.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "utils/utils_time.h"
#include "title.h"
#include "statistics/top.h"
#include "utils/utils.h"
#include "utils/id_converter.h"
#include "entities/world_objects.h"
#include "entities/zone.h"
#include "game_classes/classes_constants.h"
#include "game_magic/spells_info.h"
#include "game_magic/magic_rooms.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

using std::ifstream;
using std::fstream;

// external vars
extern bool need_warn;
extern FILE *player_fl;

extern int circle_restrict;
extern int load_into_inventory;
extern time_t zones_stat_date;
void medit_save_to_disk(int zone_num);
//extern const char *Dirs[];
// for entities
extern int check_dupes_host(DescriptorData *d, bool autocheck = false);
void do_recall(CharData *ch, char *argument, int cmd, int subcmd);
void log_zone_count_reset();
// extern functions
void appear(CharData *ch);
void reset_zone(ZoneRnum zone);
//extern CharData *find_char(long n);
void rename_char(CharData *ch, char *oname);
int _parse_name(char *arg, char *name);
int Valid_Name(char *name);
int reserved_word(const char *name);
extern bool is_empty(ZoneRnum zone_nr);
// local functions
int perform_set(CharData *ch, CharData *vict, int mode, char *val_arg);
void perform_immort_invis(CharData *ch, int level);
void do_echo(CharData *ch, char *argument, int cmd, int subcmd);
void do_send(CharData *ch, char *argument, int cmd, int subcmd);
RoomRnum find_target_room(CharData *ch, char *rawroomstr, int trig);
void do_at(CharData *ch, char *argument, int cmd, int subcmd);
void do_goto(CharData *ch, char *argument, int cmd, int subcmd);
void do_teleport(CharData *ch, char *argument, int cmd, int subcmd);
void do_vnum(CharData *ch, char *argument, int cmd, int subcmd);
void do_shutdown(CharData *ch, char *argument, int cmd, int subcmd);
void stop_snooping(CharData *ch);
void do_snoop(CharData *ch, char *argument, int cmd, int subcmd);
void do_switch(CharData *ch, char *argument, int cmd, int subcmd);
void do_return(CharData *ch, char *argument, int cmd, int subcmd);
void do_load(CharData *ch, char *argument, int cmd, int subcmd);
void do_vstat(CharData *ch, char *argument, int cmd, int subcmd);
void do_purge(CharData *ch, char *argument, int cmd, int subcmd);
void do_inspect(CharData *ch, char *argument, int cmd, int subcmd);
void do_syslog(CharData *ch, char *argument, int cmd, int subcmd);
void do_advance(CharData *ch, char *argument, int cmd, int subcmd);
void do_restore(CharData *ch, char *argument, int cmd, int subcmd);
void perform_immort_vis(CharData *ch);
void do_invis(CharData *ch, char *argument, int cmd, int subcmd);
void do_gecho(CharData *ch, char *argument, int cmd, int subcmd);
void do_poofset(CharData *ch, char *argument, int cmd, int subcmd);
void do_dc(CharData *ch, char *argument, int cmd, int subcmd);
void do_wizlock(CharData *ch, char *argument, int cmd, int subcmd);
void do_date(CharData *ch, char *argument, int cmd, int subcmd);
void do_last(CharData *ch, char *argument, int cmd, int subcmd);
void do_force(CharData *ch, char *argument, int cmd, int subcmd);
void do_wiznet(CharData *ch, char *argument, int cmd, int subcmd);
void do_zreset(CharData *ch, char *argument, int cmd, int subcmd);
void do_wizutil(CharData *ch, char *argument, int cmd, int subcmd);
void do_show(CharData *ch, char *argument, int cmd, int subcmd);
void do_set(CharData *ch, char *argument, int cmd, int subcmd);
void do_liblist(CharData *ch, char *argument, int cmd, int subcmd);
//
void do_sdemigod(CharData *ch, char *argument, int cmd, int subcmd);
void do_unfreeze(CharData *ch, char *argument, int cmd, int subcmd);
void do_setall(CharData *ch, char *argument, int cmd, int subcmd);
void do_check_occupation(CharData *ch, char *argument, int cmd, int subcmd);
void do_delete_obj(CharData *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CharData *ch, char *argument, int cmd, int subcmd);
void do_showzonestats(CharData *, char *, int, int);
void do_overstuff(CharData *ch, char *, int, int);
void do_send_text_to_char(CharData *ch, char *, int, int);
void generate_magic_enchant(ObjData *obj);

void log_zone_count_reset() {
	for (auto & i : zone_table) {
		sprintf(buf, "Zone: %d, count_reset: %d", i.vnum, i.count_reset);
		log("%s", buf);
	}
}

// Отправляет любой текст выбранному чару
void do_send_text_to_char(CharData *ch, char *argument, int, int) {
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
void do_overstuff(CharData *ch, char *, int, int) {
	std::map<std::string, int> objects;
	for (const auto & clan : Clan::ClanList) {
		for (ObjData *chest = world[real_room(clan->get_chest_room())]->contents; chest;
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
void send_to_gods(char *text, bool demigod) {
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

#define MAX_TIME 0x7fffffff

extern const char *deaf_social;

// Adds karma string to KARMA
void add_karma(CharData *ch, const char *punish, const char *reason) {
	if (reason && (reason[0] != '.')) {
		time_t nt = time(nullptr);
		sprintf(buf1, "%s :: %s [%s]\r\n", rustime(localtime(&nt)), punish, reason);
		KARMA(ch) = str_add(KARMA(ch), buf1);
	}
}

void do_delete_obj(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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

	world_objects.foreach_with_vnum(vnum, [&](const ObjData::shared_ptr &k) {
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
				if (i->vnum == vnum) {
					num++;
					sprintf(buf2, "Player %s : item \[%d] deleted\r\n", player_table[pt_num].name(), i->vnum);;
					SendMsgToChar(buf2, ch);
					i->timer = -1;
					int rnum = real_object(i->vnum);
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

void do_showzonestats(CharData *ch, char *argument, int, int) {
	std::string buffer;
	one_argument(argument, arg);
	if (!strcmp(arg, "очистить")) {
		const time_t ct = time(nullptr);
		char *date = asctime(localtime(&ct));
		SendMsgToChar(ch, "Начинаю новую запись статистики от %s", date);
		zones_stat_date = ct;
		zone_traffic_save();
		for (auto & i : zone_table) {
			i.traffic = 0;
		}
		return;
	}
	SendMsgToChar(ch,
				  "Статистика с %sДля создания новой таблицы введите команду 'очистить'.\r\n",
				  asctime(localtime(&zones_stat_date)));
	for (ZoneRnum i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
		sprintf(buf, "Zone: %5d, count_reset с ребута: %3d, посещено: %5d, назвение зоны: %s",
				zone_table[i].vnum,
				zone_table[i].count_reset,
				zone_table[i].traffic,
				zone_table[i].name);
		buffer += std::string(buf) + "\r\n";
	}
	page_string(ch->desc, buffer);
}

void do_arena_restore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	one_argument(argument, buf);
	if (!*buf)
		SendMsgToChar("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else {
		GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
		GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
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
		reset_affects(vict);
		for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (GET_EQ(vict, i)) {
				remove_otrigger(GET_EQ(vict, i), vict);
				ExtractObjFromWorld(UnequipChar(vict, i, CharEquipFlags()));
			}
		}
		ObjData *obj;
		for (obj = vict->carrying; obj; obj = vict->carrying) {
			ExtractObjFromChar(obj);
			ExtractObjFromWorld(obj);
		}
		act("Все ваши вещи были удалены и все аффекты сняты $N4!",
			false, vict, nullptr, ch, kToChar);
	}
}

int set_punish(CharData *ch, CharData *vict, int punish, char *reason, long times) {
	Punish *pundata = nullptr;
	int result;
	if (ch == vict) {
		SendMsgToChar("Это слишком жестоко...\r\n", ch);
		return 0;
	}

	if ((GetRealLevel(vict) >= kLvlImmortal && !IS_IMPL(ch)) || IS_IMPL(vict)) {
		SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
		return 0;
	}

	// Проверяем а может ли чар вообще работать с этим наказанием.
	switch (punish) {
		case SCMD_MUTE: pundata = &(vict)->player_specials->pmute;
			break;
		case SCMD_DUMB: pundata = &(vict)->player_specials->pdumb;
			break;
		case SCMD_HELL: pundata = &(vict)->player_specials->phell;
			break;
		case SCMD_NAME: pundata = &(vict)->player_specials->pname;
			break;

		case SCMD_FREEZE: pundata = &(vict)->player_specials->pfreeze;
			break;

		case SCMD_REGISTER:
		case SCMD_UNREGISTER: pundata = &(vict)->player_specials->punreg;
			break;
	}
	assert(pundata);
	if (GetRealLevel(ch) < pundata->level) {
		SendMsgToChar("Да кто ты такой!!? Чтобы оспаривать волю СТАРШИХ БОГОВ !!!\r\n", ch);
		return 0;
	}

	// Проверяем наказание или освобождение.
	if (times == 0) {
		// Чара досрочно освобождают от наказания.
		if (!reason || !*reason) {
			SendMsgToChar("Укажите причину такой милости.\r\n", ch);
			return 0;
		} else
			skip_spaces(&reason);
		//

		pundata->duration = 0;
		pundata->level = 0;
		pundata->godid = 0;

		if (pundata->reason)
			free(pundata->reason);

		pundata->reason = nullptr;

		switch (punish) {
			case SCMD_MUTE:
				if (!PLR_FLAGGED(vict, EPlrFlag::kMuted)) {
					SendMsgToChar("Ваша жертва и так может кричать.\r\n", ch);
					return (0);
				};
				PLR_FLAGS(vict).unset(EPlrFlag::kMuted);

				sprintf(buf, "Mute OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Mute OFF by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s разрешил$G вам кричать.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n2 вернулся голос.");
				break;
			case SCMD_FREEZE:
				if (!PLR_FLAGGED(vict, EPlrFlag::kFrozen)) {
					SendMsgToChar("Ваша жертва уже разморожена.\r\n", ch);
					return (0);
				};
				PLR_FLAGS(vict).unset(EPlrFlag::kFrozen);
				Glory::remove_freeze(GET_UNIQUE(vict));

				sprintf(buf, "Freeze OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Freeze OFF by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);

				if (IN_ROOM(vict) != kNowhere) {
					act("$n выпущен$a из темницы!",
						false, vict, nullptr, nullptr, kToRoom);

					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = real_room(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}
					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};

				sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n выпущен$a из темницы!");

				sprintf(buf, "%sЛедяные оковы растаяли под добрым взглядом $N1.%s",
						CCIYEL(vict, C_NRM), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n освободил$u из ледяного плена.");
				break;

			case SCMD_DUMB:
				if (!PLR_FLAGGED(vict, EPlrFlag::kDumbed)) {
					SendMsgToChar("Ваша жертва и так может издавать звуки.\r\n", ch);
					return (0);
				};
				PLR_FLAGS(vict).unset(EPlrFlag::kDumbed);

				sprintf(buf, "Dumb OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Dumb OFF by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s разрешил$G вам издавать звуки.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n нарушил$g обет молчания.");

				break;

			case SCMD_HELL:
				if (!PLR_FLAGGED(vict, EPlrFlag::kHelled)) {
					SendMsgToChar("Ваша жертва и так на свободе.\r\n", ch);
					return (0);
				};
				PLR_FLAGS(vict).unset(EPlrFlag::kHelled);

				sprintf(buf, "%s removed FROM hell by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Removed FROM hell by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);

				if (IN_ROOM(vict) != kNowhere) {
					act("$n выпущен$a из темницы!",
						false, vict, nullptr, nullptr, kToRoom);

					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = real_room(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}
					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};

				sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n выпущен$a из темницы!");
				break;

			case SCMD_NAME:

				if (!PLR_FLAGGED(vict, EPlrFlag::kNameDenied)) {
					SendMsgToChar("Вашей жертвы там нет.\r\n", ch);
					return (0);
				};
				PLR_FLAGS(vict).unset(EPlrFlag::kNameDenied);

				sprintf(buf, "%s removed FROM name room by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Removed FROM name room by %s", GET_NAME(ch));
				add_karma(vict, buf, reason);

				if (IN_ROOM(vict) != kNowhere) {
					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = real_room(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}

					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
					act("$n выпущен$a из комнаты имени!",
						false, vict, nullptr, nullptr, kToRoom);
				};
				sprintf(buf, "%s%s выпустил$G вас из комнаты имени.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n выпущен$a из комнаты имени!");
				break;

			case SCMD_REGISTER:
				// Регистриуем чара
				if (PLR_FLAGGED(vict, EPlrFlag::kRegistred)) {
					SendMsgToChar("Вашей жертва уже зарегистрирована.\r\n", ch);
					return (0);
				};

				sprintf(buf, "%s registered by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Registered by %s", GET_NAME(ch));
				RegisterSystem::add(vict, buf, reason);
				add_karma(vict, buf, reason);

				if (IN_ROOM(vict) != kNowhere) {

					act("$n зарегистрирован$a!", false, vict,
						nullptr, nullptr, kToRoom);

					if ((result = GET_LOADROOM(vict)) == kNowhere)
						result = calc_loadroom(vict);

					result = real_room(result);

					if (result == kNowhere) {
						if (GetRealLevel(vict) >= kLvlImmortal)
							result = r_immort_start_room;
						else
							result = r_mortal_start_room;
					}

					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, result);
					look_at_room(vict, result);
				};
				sprintf(buf, "%s%s зарегистрировал$G вас.%s",
						CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
				sprintf(buf2, "$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!");
				break;
			case SCMD_UNREGISTER:
				if (!PLR_FLAGGED(vict, EPlrFlag::kRegistred)) {
					SendMsgToChar("Ваша цель и так не зарегистрирована.\r\n", ch);
					return (0);
				};

				sprintf(buf, "%s unregistered by %s.", GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Unregistered by %s", GET_NAME(ch));
				RegisterSystem::remove(vict);
				add_karma(vict, buf, reason);

				if (IN_ROOM(vict) != kNowhere) {
					act("C $n1 снята метка регистрации!",
						false, vict, nullptr, nullptr, kToRoom);
					/*				if ((result = GET_LOADROOM(vict)) == kNowhere)
									result = r_unreg_start_room;

								result = real_room(result);

								if (result == kNowhere)
								{
									if (GetRealLevel(vict) >= kLevelImmortal)
										result = r_immort_start_room;
									else
										result = r_mortal_start_room;
								}

								char_from_room(vict);
								char_to_room(vict, result);
								look_at_room(vict, result);
				*/
				}
				sprintf(buf, "&W%s снял$G с вас метку регистрации.&n", GET_NAME(ch));
				sprintf(buf2, "$n лишен$g регистрации!");
				break;
		}
	} else {
		// Чара наказывают.
		if (!reason || !*reason) {
			SendMsgToChar("Укажите причину наказания.\r\n", ch);
			return 0;
		} else
			skip_spaces(&reason);

		pundata->level = PRF_FLAGGED(ch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		pundata->godid = GET_UNIQUE(ch);

		// Добавляем в причину имя имма

		sprintf(buf, "%s : %s", GET_NAME(ch), reason);
		pundata->reason = str_dup(buf);

		switch (punish) {
			case SCMD_MUTE: PLR_FLAGS(vict).set(EPlrFlag::kMuted);
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				sprintf(buf, "Mute ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Mute ON (%ldh) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s запретил$G вам кричать.%s",
						CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n подавился своим криком.");

				break;

			case SCMD_FREEZE: PLR_FLAGS(vict).set(EPlrFlag::kFrozen);
				Glory::set_freeze(GET_UNIQUE(vict));
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				sprintf(buf, "Freeze ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Freeze ON (%ldh) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%sАдский холод сковал ваше тело ледяным панцирем.\r\n%s",
						CCIBLU(vict, C_NRM), CCNRM(vict, C_NRM));
				sprintf(buf2, "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.");
				if (IN_ROOM(vict) != kNowhere) {
					act("$n водворен$a в темницу!",
						false, vict, nullptr, nullptr, kToRoom);

					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, r_helled_start_room);
					look_at_room(vict, r_helled_start_room);
				};
				break;

			case SCMD_DUMB: PLR_FLAGS(vict).set(EPlrFlag::kDumbed);
				pundata->duration = (times > 0) ? time(nullptr) + times * 60 : MAX_TIME;

				sprintf(buf, "Dumb ON for %s by %s(%ldm).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);

				sprintf(buf, "Dumb ON (%ldm) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s запретил$G вам издавать звуки.%s",
						CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

				sprintf(buf2, "$n дал$g обет молчания.");
				break;
			case SCMD_HELL: PLR_FLAGS(vict).set(EPlrFlag::kHelled);

				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				if (IN_ROOM(vict) != kNowhere) {
					act("$n водворен$a в темницу!",
						false, vict, nullptr, nullptr, kToRoom);

					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, r_helled_start_room);
					look_at_room(vict, r_helled_start_room);
				};
				vict->set_was_in_room(kNowhere);

				sprintf(buf, "%s moved TO hell by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Moved TO hell (%ldh) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s поместил$G вас в темницу.%s", GET_NAME(ch),
						CCIRED(vict, C_NRM), CCNRM(vict, C_NRM));
				sprintf(buf2, "$n водворен$a в темницу!");
				break;

			case SCMD_NAME: PLR_FLAGS(vict).set(EPlrFlag::kNameDenied);

				pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;

				if (IN_ROOM(vict) != kNowhere) {
					act("$n водворен$a в комнату имени!",
						false, vict, nullptr, nullptr, kToRoom);
					ExtractCharFromRoom(vict);
					PlaceCharToRoom(vict, r_named_start_room);
					look_at_room(vict, r_named_start_room);
				};
				vict->set_was_in_room(kNowhere);

				sprintf(buf, "%s removed to nameroom by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Removed TO nameroom (%ldh) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s поместил$G вас в комнату имени.%s",
						CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
				sprintf(buf2, "$n помещен$a в комнату имени!");
				break;

			case SCMD_UNREGISTER: pundata->duration = (times > 0) ? time(nullptr) + times * 60 * 60 : MAX_TIME;
				RegisterSystem::remove(vict);

				if (IN_ROOM(vict) != kNowhere) {
					if (vict->desc && !check_dupes_host(vict->desc) && IN_ROOM(vict) != r_unreg_start_room) {
						act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.",
							false, vict, nullptr, nullptr, kToRoom);
						ExtractCharFromRoom(vict);
						PlaceCharToRoom(vict, r_unreg_start_room);
						look_at_room(vict, r_unreg_start_room);
					}
				}
				vict->set_was_in_room(kNowhere);

				sprintf(buf, "%s unregistred by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
				mudlog(buf, DEF, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s", buf);
				sprintf(buf, "Unregistered (%ldh) by %s", times, GET_NAME(ch));
				add_karma(vict, buf, reason);

				sprintf(buf, "%s%s снял$G с вас... регистрацию :).%s",
						CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
				sprintf(buf2, "$n лишен$a регистрации!");

				break;

		}
	}
	if (ch->in_room != kNowhere) {
		act(buf, false, vict, nullptr, ch, kToChar);
		act(buf2, false, vict, nullptr, ch, kToRoom);
	};
	return 1;
}

void is_empty_ch(ZoneRnum zone_nr, CharData *ch) {
	DescriptorData *i;
	int rnum_start, rnum_stop;
	bool found = false;

	if (room_spells::IsZoneRoomAffected(zone_nr, ESpell::kRuneLabel)) {
		SendMsgToChar("В зоне имеется рунная метка.\r\n", ch);
	}

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == kNowhere)
			continue;
		if (GetRealLevel(i->character) >= kLvlImmortal)
			continue;
		if (world[i->character->in_room]->zone_rn != zone_nr)
			continue;
		sprintf(buf2,
				"Проверка по дискрипторам: В зоне (vnum: %d клетка: %d) находится персонаж: %s.\r\n",
				zone_table[zone_nr].vnum,
				GET_ROOM_VNUM(IN_ROOM(i->character)),
				GET_NAME(i->character));
		SendMsgToChar(buf2, ch);
		found = true;
	}
	if (found)
		return;
	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!get_zone_rooms(zone_nr, &rnum_start, &rnum_stop)) {
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
							GET_ROOM_VNUM(IN_ROOM(c)),
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
				GET_ROOM_VNUM(IN_ROOM(c)));
		SendMsgToChar(buf2, ch);
		found = true;
	}

	if (!found) {
		sprintf(buf2, "В зоне %d даже мышь не пробегала.\r\n", zone_table[zone_nr].vnum);
		SendMsgToChar(buf2, ch);
	}
}

void do_check_occupation(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
			is_empty_ch(zrn, ch);
			is_found = true;
			break;
		}
	}

	if (!is_found) {
		SendMsgToChar("Такой зоны нет.\r\n", ch);
	}
}

SetAllInspReqListType &setall_inspect_list = GlobalObjects::setall_inspect_list();
InspReqListType &inspect_list = GlobalObjects::inspect_list();

const int SETALL_FREEZE = 0;
const int SETALL_EMAIL = 1;
const int SETALL_PSWD = 2;
const int SETALL_HELL = 3;

void setall_inspect() {
	if (setall_inspect_list.size() == 0) {
		return;
	}
	auto it = setall_inspect_list.begin();
	CharData *ch = nullptr;
	DescriptorData *d_vict = nullptr;

	DescriptorData *imm_d = DescByUID(player_table[it->first].unique);
	if (!imm_d
		|| (STATE(imm_d) != CON_PLAYING)
		|| !(ch = imm_d->character.get())) {
		setall_inspect_list.erase(it->first);
		return;
	}

	timeval start{}, stop{}, result{};
	int is_online;
	need_warn = false;
	gettimeofday(&start, nullptr);
	Player *vict;
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++) {
		vict = new Player;
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			delete vict;
			return;
		}
		buf1[0] = '\0';
		is_online = 0;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict)
			is_online = 1;
		if (player_table[it->second->pos].mail)
			if (strstr(player_table[it->second->pos].mail, it->second->mail)) {
				it->second->found++;
				if (it->second->type_req == SETALL_FREEZE) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						set_punish(imm_d->character.get(),
								   d_vict->character.get(),
								   SCMD_FREEZE,
								   it->second->reason,
								   it->second->freeze_time);
					} else {
						if (load_char(player_table[it->second->pos].name(), vict) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							set_punish(imm_d->character.get(),
									   vict,
									   SCMD_FREEZE,
									   it->second->reason,
									   it->second->freeze_time);
							vict->save_char();
						}
					}
				} else if (it->second->type_req == SETALL_EMAIL) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						strncpy(GET_EMAIL(d_vict->character), it->second->newmail, 127);
						*(GET_EMAIL(d_vict->character) + 127) = '\0';
						sprintf(buf2,
								"Смена e-mail адреса персонажа %s с %s на %s.\r\n",
								player_table[it->second->pos].name(),
								player_table[it->second->pos].mail,
								it->second->newmail);
						add_karma(d_vict->character.get(), buf2, GET_NAME(imm_d->character));
						it->second->out += buf2;

					} else {
						if (load_char(player_table[it->second->pos].name(), vict) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								it->second->out += buf1;
								continue;
							}
							strncpy(GET_EMAIL(vict), it->second->newmail, 127);
							*(GET_EMAIL(vict) + 127) = '\0';
							sprintf(buf2,
									"Смена e-mail адреса персонажа %s с %s на %s.\r\n",
									player_table[it->second->pos].name(),
									player_table[it->second->pos].mail,
									it->second->newmail);
							it->second->out += buf2;
							add_karma(vict, buf2, GET_NAME(imm_d->character));
							vict->save_char();
						}
					}
				} else if (it->second->type_req == SETALL_PSWD) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						Password::set_password(d_vict->character.get(), std::string(it->second->pwd));
						sprintf(buf2, "У персонажа %s изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += buf2;
						sprintf(buf1, "\r\n");
						it->second->out += buf1;
						add_karma(d_vict->character.get(), buf2, GET_NAME(imm_d->character));
					} else {
						if (load_char(player_table[it->second->pos].name(), vict) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						}
						if (GetRealLevel(vict) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						Password::set_password(vict, std::string(it->second->pwd));
						std::string str = player_table[it->second->pos].name();
						str[0] = UPPER(str[0]);
						sprintf(buf2, "У персонажа %s изменен пароль (setall).", player_table[it->second->pos].name());
						it->second->out += buf2;
						sprintf(buf1, "\r\n");
						it->second->out += buf1;
						add_karma(vict, buf2, GET_NAME(imm_d->character));
						vict->save_char();
					}
				} else if (it->second->type_req == SETALL_HELL) {
					if (is_online) {
						if (GetRealLevel(d_vict->character) >= kLvlGod) {
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						set_punish(imm_d->character.get(),
								   d_vict->character.get(),
								   SCMD_HELL,
								   it->second->reason,
								   it->second->freeze_time);
					} else {
						if (load_char(player_table[it->second->pos].name(), vict) < 0) {
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						} else {
							if (GetRealLevel(vict) >= kLvlGod) {
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							set_punish(imm_d->character.get(),
									   vict,
									   SCMD_HELL,
									   it->second->reason,
									   it->second->freeze_time);
							vict->save_char();
						}
					}
				}
			}
		delete vict;
	}
	if (it->second->mail && it->second->pwd)
		Password::send_password(it->second->mail, it->second->pwd);
	// освобождение памяти
	if (it->second->pwd)
		free(it->second->pwd);
	if (it->second->reason)
		free(it->second->reason);
	if (it->second->newmail)
		free(it->second->newmail);
	if (it->second->mail)
		free(it->second->mail);
	need_warn = true;
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d.\r\n", it->second->found);
	it->second->out += buf1;
	page_string(ch->desc, it->second->out);
	setall_inspect_list.erase(it->first);
}

void do_setall(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int type_request = 0;
	int times = 0;
	if (ch->get_pfilepos() < 0)
		return;

	auto it_inspect = inspect_list.find(GET_UNIQUE(ch));
	auto it = setall_inspect_list.find(GET_UNIQUE(ch));
	// На всякий случай разрешаем только одну команду такого типа - либо setall, либо inspect
	if (it_inspect != inspect_list.end() && it != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}

	argument = three_arguments(argument, buf, buf1, buf2);
	SetAllInspReqPtr req(new setall_inspect_request);
	req->newmail = nullptr;
	req->mail = nullptr;
	req->reason = nullptr;
	req->pwd = nullptr;

	if (!*buf) {
		SendMsgToChar("Usage: setall <e-mail> <email|passwd|frozen|hell> <arguments>\r\n", ch);
		return;
	}

	if (!IsValidEmail(buf)) {
		SendMsgToChar("Некорректный e-mail!\r\n", ch);
		return;
	}

	if (!isname(buf1, "frozen email passwd hell")) {
		SendMsgToChar("Данное действие совершить нельзя.\r\n", ch);
		return;
	}
	if (utils::IsAbbrev(buf1, "frozen")) {
		skip_spaces(&argument);
		if (!argument || !*argument) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = SETALL_FREEZE;
		req->freeze_time = times;
		req->reason = strdup(argument);
	} else if (utils::IsAbbrev(buf1, "email")) {
		if (!*buf2) {
			SendMsgToChar("Укажите новый e-mail!\r\n", ch);
			return;
		}
		if (!IsValidEmail(buf2)) {
			SendMsgToChar("Новый e-mail некорректен!\r\n", ch);
			return;
		}
		req->newmail = strdup(buf2);
		type_request = SETALL_EMAIL;
	} else if (utils::IsAbbrev(buf1, "passwd")) {
		if (!*buf2) {
			SendMsgToChar("Укажите новый пароль!\r\n", ch);
			return;
		}
		req->pwd = strdup(buf2);
		type_request = SETALL_PSWD;
	} else if (utils::IsAbbrev(buf1, "hell")) {
		skip_spaces(&argument);
		if (!argument || !*argument) {
			SendMsgToChar("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = SETALL_HELL;
		req->freeze_time = times;
		req->reason = strdup(argument);
	} else {
		SendMsgToChar("Какой-то баг. Вы эту надпись видеть не должны.\r\n", ch);
		return;
	}

	req->type_req = type_request;
	req->mail = str_dup(buf);
	req->pos = 0;
	req->found = 0;
	req->out = "";
	setall_inspect_list[ch->get_pfilepos()] = req;
}

void do_echo(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		SendMsgToChar("Вы не в состоянии что-либо продемонстрировать окружающим.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("И что вы хотите выразить столь красочно?\r\n", ch);
	} else {
		if (subcmd == SCMD_EMOTE) {
			// added by Pereplut
			if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed)) {
				if PLR_FLAGGED(ch->get_master(), EPlrFlag::kDumbed) {
					// shapirus: правильно пишется не "так-же", а "так же".
					// и запятая пропущена была :-P.
					SendMsgToChar("Ваши последователи так же немы, как и вы!\r\n", ch->get_master());
					return;
				}
			}
			sprintf(buf, "&K$n %s.&n", argument);
		} else {
			strcpy(buf, argument);
		}

		for (const auto to : world[ch->in_room]->people) {
			if (to == ch
				|| ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}

			act(buf, false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}

		if (PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
			SendMsgToChar(OK, ch);
		} else {
			act(buf, false, ch, nullptr, nullptr, kToChar);
		}
	}
}

#define SHOW_GLORY    0
#define ADD_GLORY    1
#define SUB_GLORY    2
#define SUB_STATS    3
#define SUB_TRANS    4
#define SUB_HIDE    5

void do_glory(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	// Команда простановки славы (оффлайн/онлайн)
	// Без параметров выводит славу у игрока
	// + cлава прибавляет славу
	// - cлава убавляет славу
	char num[kMaxInputLength];
	char arg1[kMaxInputLength];
	int mode = 0;
	char *reason;

	if (!*argument) {
		SendMsgToChar("Формат команды : \r\n"
					 "   glory <имя> +|-<кол-во славы> причина\r\n"
					 "   glory <имя> remove <кол-во статов> причина (снимание уже вложенной чаром славы)\r\n"
					 "   glory <имя> transfer <имя принимаюего славу> причина (переливание славы на другого чара)\r\n"
					 "   glory <имя> hide on|off причина (показывать или нет чара в топе славы)\r\n", ch);
		return;
	}
	reason = two_arguments(argument, arg, num);
	skip_spaces(&reason);

	if (!*num)
		mode = SHOW_GLORY;
	else if (*num == '+')
		mode = ADD_GLORY;
	else if (*num == '-')
		mode = SUB_GLORY;
	else if (utils::IsAbbrev(num, "remove")) {
		// тут у нас в num получается remove, в arg1 кол-во и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_STATS;
	} else if (utils::IsAbbrev(num, "transfer")) {
		// а тут в num transfer, в arg1 имя принимающего славу и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_TRANS;
	} else if (utils::IsAbbrev(num, "hide")) {
		// а тут в num hide, в arg1 on|off и в reason причина
		reason = any_one_arg(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_HIDE;
	}

	// точки убираем, чтобы карма всегда писалась
	skip_dots(&reason);

	if (mode != SHOW_GLORY) {
		if ((reason == nullptr) || (*reason == 0)) {
			SendMsgToChar("Укажите причину изменения славы?\r\n", ch);
			return;
		}
	}

	CharData *vict = get_player_vis(ch, arg, EFind::kCharInWorld);
	Player t_vict; // TODO: надо выносить во вторую функцию, чтобы зря не создавать
	if (!vict) {
		if (load_char(arg, &t_vict) < 0) {
			SendMsgToChar("Такого персонажа не существует.\r\n", ch);
			return;
		}
		vict = &t_vict;
	}

	switch (mode) {
		case ADD_GLORY: {
			int amount = atoi((num + 1));
			Glory::add_glory(GET_UNIQUE(vict), amount);
			SendMsgToChar(ch, "%s добавлено %d у.е. славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 2), amount, Glory::get_glory(GET_UNIQUE(vict)));
			imm_log("(GC) %s sets +%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			// запись в карму
			sprintf(buf, "Change glory +%d by %s", amount, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case SUB_GLORY: {
			int amount = Glory::remove_glory(GET_UNIQUE(vict), atoi((num + 1)));
			if (amount <= 0) {
				SendMsgToChar(ch, "У %s нет свободной славы.", GET_PAD(vict, 1));
				break;
			}
			SendMsgToChar(ch, "У %s вычтено %d у.е. славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 1), amount, Glory::get_glory(GET_UNIQUE(vict)));
			imm_log("(GC) %s sets -%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			// запись в карму
			sprintf(buf, "Change glory -%d by %s", amount, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case SUB_STATS: {
			if (Glory::remove_stats(vict, ch, atoi(arg1))) {
				sprintf(buf, "Remove stats %s by %s", arg1, GET_NAME(ch));
				add_karma(vict, buf, reason);
				GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			}
			break;
		}
		case SUB_TRANS: {
			Glory::transfer_stats(vict, ch, arg1, reason);
			break;
		}
		case SUB_HIDE: {
			Glory::hide_char(vict, ch, arg1);
			sprintf(buf, "Hide %s by %s", arg1, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			break;
		}
		default: Glory::show_glory(vict, ch);
	}

	vict->save_char();
}

void do_send(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
	if (PRF_FLAGGED(ch, EPrf::kNoRepeat))
		SendMsgToChar("Послано.\r\n", ch);
	else {
		snprintf(buf2, kMaxStringLength, "Вы послали '%s' %s.\r\n", buf, GET_PAD(vict, 2));
		SendMsgToChar(buf2, ch);
	}
}

// take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93
RoomRnum find_target_room(CharData *ch, char *rawroomstr, int trig) {
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
		if ((location = real_room(tmp)) == kNowhere) {
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
	if (!IS_GRGOD(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
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

void do_at(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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

	if ((location = find_target_room(ch, buf, 0)) == kNowhere)
		return;

	// a location has been found.
	original_loc = ch->in_room;
	ExtractCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	command_interpreter(ch, command);

	// check if the char is still there
	if (ch->in_room == location) {
		ExtractCharFromRoom(ch);
		PlaceCharToRoom(ch, original_loc);
	}
	ch->dismount();
}

void do_unfreeze(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
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
		if (load_char(name_buffer.c_str(), &t_vict) < 0) {
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
		set_punish(ch, vict, SCMD_FREEZE, reason_c, 0);
		vict->save_char();
		sprintf(buf, "Чар %s разморожен.\r\n", name_buffer.c_str());
		SendMsgToChar(buf, ch);
	}

	delete[] reason_c;
	unfreeze_list.close();

}

void do_goto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum location;

	if ((location = find_target_room(ch, argument, 0)) == kNowhere)
		return;

	if (POOFOUT(ch))
		sprintf(buf, "$n %s", POOFOUT(ch));
	else
		strcpy(buf, "$n растворил$u в клубах дыма.");

	act(buf, true, ch, nullptr, nullptr, kToRoom);
	ExtractCharFromRoom(ch);

	PlaceCharToRoom(ch, location);
	ch->dismount();

	if (POOFIN(ch))
		sprintf(buf, "$n %s", POOFIN(ch));
	else
		strcpy(buf, "$n возник$q посреди комнаты.");
	act(buf, true, ch, nullptr, nullptr, kToRoom);
	look_at_room(ch, 0);
}

void do_teleport(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	RoomRnum target;

	two_arguments(argument, buf, buf2);

	if (!*buf)
		SendMsgToChar("Кого вы хотите переместить?\r\n", ch);
	else if (!(victim = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else if (victim == ch)
		SendMsgToChar("Используйте 'прыжок' для собственного перемещения.\r\n", ch);
	else if (GetRealLevel(victim) >= GetRealLevel(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))
		SendMsgToChar("Попробуйте придумать что-то другое.\r\n", ch);
	else if (!*buf2)
		act("Куда вы хотите $S переместить?", false, ch, nullptr, victim, kToChar);
	else if ((target = find_target_room(ch, buf2, 0)) != kNowhere) {
		SendMsgToChar(OK, ch);
		act("$n растворил$u в клубах дыма.", false, victim, nullptr, nullptr, kToRoom);
		ExtractCharFromRoom(victim);
		PlaceCharToRoom(victim, target);
		victim->dismount();
		act("$n появил$u, окутанн$w розовым туманом.",
			false, victim, nullptr, nullptr, kToRoom);
		act("$n переместил$g вас!", false, ch, nullptr, (char *) victim, kToVict);
		look_at_room(victim, 0);
	}
}

void do_vnum(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2
		|| (!utils::IsAbbrev(buf, "mob") && !utils::IsAbbrev(buf, "obj") && !utils::IsAbbrev(buf, "room") && !utils::IsAbbrev(buf, "flag")
			&& !utils::IsAbbrev(buf, "существо") && !utils::IsAbbrev(buf, "предмет") && !utils::IsAbbrev(buf, "флаг")
			&& !utils::IsAbbrev(buf, "комната")
			&& !utils::IsAbbrev(buf, "trig") && !utils::IsAbbrev(buf, "триггер"))) {
		SendMsgToChar("Usage: vnum { obj | mob | flag | room | trig } <name>\r\n", ch);
		return;
	}

	if ((utils::IsAbbrev(buf, "mob")) || (utils::IsAbbrev(buf, "существо")))
		if (!vnum_mobile(buf2, ch))
			SendMsgToChar("Нет существа с таким именем.\r\n", ch);

	if ((utils::IsAbbrev(buf, "obj")) || (utils::IsAbbrev(buf, "предмет")))
		if (!vnum_object(buf2, ch))
			SendMsgToChar("Нет предмета с таким названием.\r\n", ch);

	if ((utils::IsAbbrev(buf, "flag")) || (utils::IsAbbrev(buf, "флаг")))
		if (!vnum_flag(buf2, ch))
			SendMsgToChar("Нет объектов с таким флагом.\r\n", ch);

	if ((utils::IsAbbrev(buf, "room")) || (utils::IsAbbrev(buf, "комната")))
		if (!vnum_room(buf2, ch))
			SendMsgToChar("Нет объектов с таким флагом.\r\n", ch);

	if (utils::IsAbbrev(buf, "trig") || utils::IsAbbrev(buf, "триггер"))
		if (!vnum_obj_trig(buf2, ch))
			SendMsgToChar("Нет триггеров, загружаемых такой объект\r\n", ch);
}

void do_shutdown(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	commands::Shutdown command(ch, argument, shutdown_parameters);
	if (command.parse_arguments()) {
		command.execute();
	}
}

void stop_snooping(CharData *ch) {
	if (!ch->desc->snooping)
		SendMsgToChar("Вы не подслушиваете.\r\n", ch);
	else {
		SendMsgToChar("Вы прекратили подслушивать.\r\n", ch);
		ch->desc->snooping->snoop_by = nullptr;
		ch->desc->snooping = nullptr;
	}
}

void do_snoop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim, *tch;

	if (!ch->desc)
		return;

	argument = one_argument(argument, arg);

	if (!*arg)
		stop_snooping(ch);
	else if (!(victim = get_player_vis(ch, arg, EFind::kCharInWorld)))
		SendMsgToChar("Нет такого создания в игре.\r\n", ch);
	else if (!victim->desc)
		act("Вы не можете $S подслушать - он$G потерял$G связь..\r\n",
			false, ch, nullptr, victim, kToChar);
	else if (victim == ch)
		stop_snooping(ch);
	else if (victim->desc->snooping == ch->desc)
		SendMsgToChar("Вы уже подслушиваете.\r\n", ch);
	else if (victim->desc->snoop_by && victim->desc->snoop_by != ch->desc)
		SendMsgToChar("Дык его уже кто-то из богов подслушивает.\r\n", ch);
		//	else if (!can_snoop(ch, victim))
		//		SendMsgToChar("Дружина данного персонажа находится в состоянии войны с вашей дружиной.\r\n", ch);
	else {
		if (victim->desc->original)
			tch = victim->desc->original.get();
		else
			tch = victim;

		const int god_level = PRF_FLAGGED(ch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		const int victim_level = PRF_FLAGGED(tch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(tch);

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

void do_switch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
			&& ROOM_FLAGGED(IN_ROOM(visible_character), ERoomFlag::kGodsRoom)) {
			SendMsgToChar("Вы не можете находиться в той комнате.\r\n", ch);
		} else if (!IS_GRGOD(ch)
			&& !Clan::MayEnter(ch, IN_ROOM(visible_character), kHousePortal)) {
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

void do_return(CharData *ch, char *argument, int cmd, int subcmd) {
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

void do_load(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *mob;
	MobVnum number;
	MobRnum r_num;
	char *iname;

	iname = two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !a_isdigit(*buf2)) {
		SendMsgToChar("Usage: load { obj | mob } <number>\r\n"
					  "       load ing { <сила> | <VNUM> } <имя>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0) {
		SendMsgToChar("Отрицательный моб опасен для вашего здоровья!\r\n", ch);
		return;
	}
	if (utils::IsAbbrev(buf, "mob")) {
		if ((r_num = real_mobile(number)) < 0) {
			SendMsgToChar("Нет такого моба в этом МУДе.\r\n", ch);
			return;
		}
		if ((zone_table[get_zone_rnum_by_mob_vnum(number)].locked) && (GetRealLevel(ch) != kLvlImplementator)) {
			SendMsgToChar("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		PlaceCharToRoom(mob, ch->in_room);
		act("$n порыл$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $N3!", false, ch, nullptr, mob, kToRoom);
		act("Вы создали $N3.", false, ch, nullptr, mob, kToChar);
		load_mtrigger(mob);
		olc_log("%s load mob %s #%d", GET_NAME(ch), GET_NAME(mob), number);
	} else if (utils::IsAbbrev(buf, "obj")) {
		if ((r_num = real_object(number)) < 0) {
			SendMsgToChar("Господи, да изучи ты номера объектов.\r\n", ch);
			return;
		}
		if ((zone_table[get_zone_rnum_by_obj_vnum(number)].locked) && (GetRealLevel(ch) != kLvlImplementator)) {
			SendMsgToChar("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		obj->set_crafter_uid(GET_UNIQUE(ch));

		if (number == GlobalDrop::MAGIC1_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC2_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC3_ENCHANT_VNUM) {
			generate_magic_enchant(obj.get());
		}

		if (load_into_inventory) {
			PlaceObjToInventory(obj.get(), ch);
		} else {
			PlaceObjToRoom(obj.get(), ch->in_room);
		}

		act("$n покопал$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $o3!", false, ch, obj.get(), nullptr, kToRoom);
		act("Вы создали $o3.", false, ch, obj.get(), nullptr, kToChar);
		load_otrigger(obj.get());
		CheckObjDecay(obj.get());
		olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), number);
	} else if (utils::IsAbbrev(buf, "ing")) {
		int power, i;
		power = atoi(buf2);
		skip_spaces(&iname);
		i = im_get_type_by_name(iname, 0);
		if (i < 0) {
			SendMsgToChar("Неверное имя типа\r\n", ch);
			return;
		}
		const auto obj = load_ingredient(i, power, power);
		if (!obj) {
			SendMsgToChar("Ошибка загрузки ингредиента\r\n", ch);
			return;
		}
		PlaceObjToInventory(obj, ch);
		act("$n покопал$u в МУДе.", true, ch, nullptr, nullptr, kToRoom);
		act("$n создал$g $o3!", false, ch, obj, nullptr, kToRoom);
		act("Вы создали $o3.", false, ch, obj, nullptr, kToChar);
		sprintf(buf, "%s load ing %d %s", GET_NAME(ch), power, iname);
		mudlog(buf, NRM, kLvlBuilder, IMLOG, true);
		load_otrigger(obj);
		CheckObjDecay(obj);
		olc_log("%s load ing %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), power);
	} else {
		SendMsgToChar("Нет уж. Ты создай че-нить нормальное.\r\n", ch);
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

void do_vstat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *mob;
	MobVnum number;    // or ObjVnum ...
	MobRnum r_num;        // or ObjRnum ...

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !a_isdigit(*buf2)) {
		SendMsgToChar("Usage: vstat { obj | mob } <number>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0) {
		SendMsgToChar("Отрицательный номер? Оригинально!\r\n", ch);
		return;
	}
	if (utils::IsAbbrev(buf, "mob")) {
		if ((r_num = real_mobile(number)) < 0) {
			SendMsgToChar("Обратитесь в Арктику - там ОН живет.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		PlaceCharToRoom(mob, 1);
		do_stat_character(ch, mob, 1);
		ExtractCharFromWorld(mob, false);
	} else if (utils::IsAbbrev(buf, "obj")) {
		if ((r_num = real_object(number)) < 0) {
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
void do_purge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	ObjData *obj, *next_o;

	one_argument(argument, buf);

	if (*buf) {        // argument supplied. destroy single object or char
		if ((vict = get_char_vis(ch, buf, EFind::kCharInRoom)) != nullptr) {
			if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
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
			// TODO: честно говоря дублирование куска из экстракта не ясно
			// смену лидера пока сюду не сую, над вникнуть будет...
			if (vict->followers
				|| vict->has_master()) {
				die_follower(vict);
			}

			if (!vict->purged()) {
				ExtractCharFromWorld(vict, false);
			}
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

		for (obj = world[ch->in_room]->contents; obj; obj = next_o) {
			next_o = obj->get_next_content();
			ExtractObjFromWorld(obj);
		}
	}
}

void send_list_char(std::string list_char, std::string email) {
	std::string cmd_line = "python3 send_list_char.py " + email + " " + list_char + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

const int IIP = 1;
const int IMAIL = 2;
const int ICHAR = 3;

char *show_pun_time(int time) {
	static char time_buf[16];
	time_buf[0] = '\0';
	if (time < 3600)
		snprintf(time_buf, sizeof(time_buf), "%d m", (int) time / 60);
	else if (time < 3600 * 24)
		snprintf(time_buf, sizeof(time_buf), "%d h", (int) time / 3600);
	else if (time < 3600 * 24 * 30)
		snprintf(time_buf, sizeof(time_buf), "%d D", (int) time / (3600 * 24));
	else if (time < 3600 * 24 * 365)
		snprintf(time_buf, sizeof(time_buf), "%d M", (int) time / (3600 * 24 * 30));
	else
		snprintf(time_buf, sizeof(time_buf), "%d Y", (int) time / (3600 * 24 * 365));
	return time_buf;
}
//выводим наказания для чара
void show_pun(CharData *vict, char *buf) {
	if (PLR_FLAGGED(vict, EPlrFlag::kFrozen)
		&& FREEZE_DURATION(vict))
		sprintf(buf + strlen(buf), "FREEZE : %s [%s].\r\n",
				show_pun_time(FREEZE_DURATION(vict) - time(nullptr)),
				FREEZE_REASON(vict) ? FREEZE_REASON(vict)
									: "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kMuted)
		&& MUTE_DURATION(vict))
		sprintf(buf + strlen(buf), "MUTE   : %s [%s].\r\n",
				show_pun_time(MUTE_DURATION(vict) - time(nullptr)),
				MUTE_REASON(vict) ? MUTE_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kDumbed)
		&& DUMB_DURATION(vict))
		sprintf(buf + strlen(buf), "DUMB   : %s [%s].\r\n",
				show_pun_time(DUMB_DURATION(vict) - time(nullptr)),
				DUMB_REASON(vict) ? DUMB_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, EPlrFlag::kHelled)
		&& HELL_DURATION(vict))
		sprintf(buf + strlen(buf), "HELL   : %s [%s].\r\n",
				show_pun_time(HELL_DURATION(vict) - time(nullptr)),
				HELL_REASON(vict) ? HELL_REASON(vict) : "-");

	if (!PLR_FLAGGED(vict, EPlrFlag::kRegistred)
		&& UNREG_DURATION(vict))
		sprintf(buf + strlen(buf), "UNREG  : %s [%s].\r\n",
				show_pun_time(UNREG_DURATION(vict) - time(nullptr)),
				UNREG_REASON(vict) ? UNREG_REASON(vict) : "-");
}

void inspecting() {
	if (inspect_list.empty()) {
		return;
	}

	auto it = inspect_list.begin();

	CharData *ch = nullptr;
	DescriptorData *d_vict = nullptr;

	//если нет дескриптора или он где-то там по меню шарится, то запрос отменяется
	if (!(d_vict = DescByUID(player_table[it->first].unique))
		|| (STATE(d_vict) != CON_PLAYING)
		|| !(ch = d_vict->character.get())) {
		inspect_list.erase(it->first);
		return;
	}

	timeval start{}, stop{}, result{};
	time_t mytime;
	int mail_found = 0;
	int is_online;
	need_warn = false;

	gettimeofday(&start, nullptr);
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++) {
		gettimeofday(&stop, nullptr);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= kOptUsec) {
			return;
		}

#ifdef TEST_BUILD
		log("inspecting %d/%lu", 1 + it->second->pos, player_table.size());
#endif

		if (!*it->second->req) {
			SendMsgToChar(ch,
						  "Ошибка: пустой параметр для поиска");//впринципе никогда не должно вылезти, но на всякий случай воткнул проверку
			break;
		}

		if ((it->second->sfor == ICHAR
			&& it->second->unique == player_table[it->second->pos].unique)//Это тот же перс которого мы статим
			|| (player_table[it->second->pos].level >= kLvlImmortal && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[it->second->pos].level > GetRealLevel(ch) && !IS_IMPL(ch)
				&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)))//если левел больше то облом
		{
			continue;
		}

		buf1[0] = '\0';
		buf2[0] = '\0';
		is_online = 0;

		CharData::shared_ptr vict;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict) {
			is_online = 1;
		}

		if (it->second->sfor != IMAIL && it->second->fullsearch) {
			if (d_vict) {
				vict = d_vict->character;
			} else {
				vict.reset(new Player);
				if (load_char(player_table[it->second->pos].name(), vict.get()) < 0) {
					SendMsgToChar(ch,
								  "Некорректное имя персонажа (%s) inspecting %s: %s.\r\n",
								  player_table[it->second->pos].name(),
								  (it->second->sfor == IMAIL ? "mail" : (it->second->sfor == IIP ? "ip" : "char")),
								  it->second->req);
					continue;
				}
			}

			show_pun(vict.get(), buf2);
		}

		if (it->second->sfor == IMAIL || it->second->sfor == ICHAR) {
			mail_found = 0;
			if (player_table[it->second->pos].mail) {
				if ((it->second->sfor == IMAIL
					&& strstr(player_table[it->second->pos].mail, it->second->req))
					|| (it->second->sfor == ICHAR && !strcmp(player_table[it->second->pos].mail, it->second->mail))) {
					mail_found = 1;
				}
			}
		}

		if (it->second->sfor == IIP
			|| it->second->sfor == ICHAR) {
			if (!it->second->fullsearch) {
				if (player_table[it->second->pos].last_ip) {
					if ((it->second->sfor == IIP
						&& strstr(player_table[it->second->pos].last_ip, it->second->req))
						|| (!it->second->ip_log.empty()
							&& !str_cmp(player_table[it->second->pos].last_ip, it->second->ip_log.at(0).ip))) {
						sprintf(buf1 + strlen(buf1),
								" IP:%s%-16s%s\r\n",
								(it->second->sfor == ICHAR ? CCBLU(ch, C_SPR) : ""),
								player_table[it->second->pos].last_ip,
								(it->second->sfor == ICHAR ? CCNRM(ch, C_SPR) : ""));
					}
				}
			} else if (vict && !LOGON_LIST(vict).empty()) {
				for (const auto &cur_log : LOGON_LIST(vict)) {
					for (const auto &ch_log : it->second->ip_log) {
						if (!ch_log.ip) {
							SendMsgToChar(ch,
										  "Ошибка: пустой ip\r\n");//поиск прерываеться если криво заполнено поле ip для поиска
							break;
						}

						if (!str_cmp(cur_log.ip, ch_log.ip)) {
							sprintf(buf1 + strlen(buf1),
									" IP:%s%-16s%s Количество входов с него:%5ld Последний раз: %-30s\r\n",
									CCBLU(ch, C_SPR),
									cur_log.ip,
									CCNRM(ch, C_SPR),
									cur_log.count,
									rustime(localtime(&cur_log.lasttime)));
/*							if (it->second->sfor == ICHAR)
							{
								sprintf(buf1 + strlen(buf1), "-> Count:%5ld Last : %s\r\n",
									ch_log.count, rustime(localtime(&ch_log.lasttime)));
							}*/
						}
					}
				}
			}
		}

		if (*buf1 || mail_found) {
			const auto &player = player_table[it->second->pos];
			strcpy(smallBuf, MUD::Classes(player.plr_class).GetCName());
			mytime = player_table[it->second->pos].last_logon;
			Player vict;
			char clanstatus[kMaxInputLength];
			sprintf(clanstatus, "%s", "нет");
			if ((load_char(player.name(), &vict)) > -1) {
				Clan::SetClanData(&vict);
				if (CLAN(&vict))
					sprintf(clanstatus, "%s", (&vict)->player_specials->clan->GetAbbrev());
			}
			sprintf(buf,
					"--------------------\r\nИмя: %s%-12s%s e-mail: %s&S%-30s&s%s Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
					(is_online ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
					player.name(),
					CCNRM(ch, C_SPR),
					(mail_found && it->second->sfor != IMAIL ? CCBLU(ch, C_SPR) : ""),
					player.mail,
					(mail_found ? CCNRM(ch, C_SPR) : ""),
					rustime(localtime(&mytime)),
					player.level,
					player.remorts,
					smallBuf, clanstatus);
			it->second->out += buf;
			it->second->out += buf2;
			it->second->out += buf1;
			it->second->found++;
		}
	}

	need_warn = true;
	gettimeofday(&stop, nullptr);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d за %ldсек.\r\n", it->second->found, result.tv_sec);
	it->second->out += buf1;
	if (it->second->sendmail)
		if (it->second->found > 1 && it->second->sfor == IMAIL) {
			it->second->out += "Данный список отправлен игроку на емайл\r\n";
			send_list_char(it->second->out, it->second->req);
		}
	if (it->second->mail)
		free(it->second->mail);

	page_string(ch->desc, it->second->out);
	free(it->second->req);
	inspect_list.erase(it->first);
}

//added by WorM Команда для поиска чаров с одинаковым(похожим) mail и/или ip
void do_inspect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d_vict = nullptr;
	int i = 0;

	if (ch->get_pfilepos() < 0)
		return;

	auto it = inspect_list.find(GET_UNIQUE(ch));
	auto it_setall = setall_inspect_list.find(GET_UNIQUE(ch));
	// Навсякий случай разрешаем только одну команду такого типа, либо сетол, либо инспект
	if (it != inspect_list.end() && it_setall != setall_inspect_list.end()) {
		SendMsgToChar(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}
	argument = two_arguments(argument, buf, buf2);
	if (!*buf || !*buf2 || !a_isascii(*buf2)) {
		SendMsgToChar("Usage: inspect { mail | ip | char } <argument> [all|все|sendmail]\r\n", ch);
		return;
	}
	if (!isname(buf, "mail ip char")) {
		SendMsgToChar("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if (strlen(buf2) < 3) {
		SendMsgToChar("Слишком короткий запрос\r\n", ch);
		return;
	}
	if (strlen(buf2) > 65) {
		SendMsgToChar("Слишком длинный запрос\r\n", ch);
		return;
	}
	if (utils::IsAbbrev(buf, "char") && (GetUniqueByName(buf2) <= 0)) {
		SendMsgToChar(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", buf2);
		return;
	}
	InspReqPtr req(new inspect_request);
	req->mail = nullptr;
	req->fullsearch = 0;
	req->req = str_dup(buf2);
	req->sendmail = false;
	buf2[0] = '\0';

	if (argument) {
		if (isname(argument, "все all"))
			if (IS_GRGOD(ch) || PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				need_warn = false;
				req->fullsearch = 1;
			}
		if (isname(argument, "sendmail"))
			req->sendmail = true;
	}
	if (utils::IsAbbrev(buf, "mail")) {
		req->sfor = IMAIL;
	} else if (utils::IsAbbrev(buf, "ip")) {
		req->sfor = IIP;
		if (req->fullsearch) {
			const Logon logon = {str_dup(req->req), 0, 0, false};
			req->ip_log.push_back(logon);
		}
	} else if (utils::IsAbbrev(buf, "char")) {
		req->sfor = ICHAR;
		req->unique = GetUniqueByName(req->req);
		i = get_ptable_by_unique(req->unique);
		if ((req->unique <= 0)//Перс не существует
			|| (player_table[i].level >= kLvlImmortal && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[i].level > GetRealLevel(ch) && !IS_IMPL(ch)
				&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)))//если левел больше то облом
		{
			SendMsgToChar(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", req->req);
			req.reset();
			return;
		}

		d_vict = DescByUID(req->unique);
		req->mail = str_dup(player_table[i].mail);
		time_t tmp_time = player_table[i].last_logon;

		sprintf(buf,
				"Персонаж: %s%s%s e-mail: %s&S%s&s%s Last: %s%s%s from IP: %s%s%s\r\n",
				(d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
				player_table[i].name(),
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				req->mail,
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				rustime(localtime(&tmp_time)),
				CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR),
				player_table[i].last_ip,
				CCNRM(ch, C_SPR));
		Player vict;
		char clanstatus[kMaxInputLength];
		sprintf(clanstatus, "%s", "нет");
		if ((load_char(player_table[i].name(), &vict)) > -1) {
			Clan::SetClanData(&vict);
			if (CLAN(&vict))
				sprintf(clanstatus, "%s", (&vict)->player_specials->clan->GetAbbrev());
		}
		strcpy(smallBuf, MUD::Classes(player_table[i].plr_class).GetCName());
		time_t mytime = player_table[i].last_logon;
		sprintf(buf1, "Last: %s. Level %d, Remort %d, Проф: %s, Клан: %s.\r\n",
				rustime(localtime(&mytime)),
				player_table[i].level, player_table[i].remorts, smallBuf, clanstatus);
		strcat(buf, buf1);

		if (req->fullsearch) {
			CharData::shared_ptr vict;
			if (d_vict) {
				vict = d_vict->character;
			} else {
				vict.reset(new Player);
				if (load_char(req->req, vict.get()) < 0) {
					SendMsgToChar(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", req->req);
					return;
				}
			}

			show_pun(vict.get(), buf2);
			if (vict && !LOGON_LIST(vict).empty()) {
#ifdef TEST_BUILD
				log("filling logon list");
#endif
				for (const auto &cur_log : LOGON_LIST(vict)) {
					const Logon logon = {str_dup(cur_log.ip), cur_log.count, cur_log.lasttime, false};
					req->ip_log.push_back(logon);
				}
			}
		} else {
			const Logon logon = {str_dup(player_table[i].last_ip), 0, player_table[i].last_logon, false};
			req->ip_log.push_back(logon);
		}
	}

	if (req->sfor < ICHAR) {
		sprintf(buf, "%s: %s&S%s&s%s\r\n", (req->sfor == IIP ? "IP" : "e-mail"),
				CCWHT(ch, C_SPR), req->req, CCNRM(ch, C_SPR));
	}
	req->pos = 0;
	req->found = 0;
	req->out += buf;
	req->out += buf2;

	gettimeofday(&req->start, nullptr);
	inspect_list[ch->get_pfilepos()] = req;
}

const char *logtypes[] =
	{
		"нет", "начальный", "краткий", "нормальный", "полный", "\n"
	};

// subcmd - канал
void do_syslog(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
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
	return;
}

void do_advance(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char *name = arg, *level = buf2;
	int newlevel, oldlevel;

	two_arguments(argument, name, level);

	if (*name) {
		if (!(victim = get_player_vis(ch, name, EFind::kCharInWorld))) {
			SendMsgToChar("Не найду такого игрока.\r\n", ch);
			return;
		}
	} else {
		SendMsgToChar("Повысить кого?\r\n", ch);
		return;
	}

	if (GetRealLevel(ch) <= GetRealLevel(victim) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		SendMsgToChar("Нелогично.\r\n", ch);
		return;
	}
	if (!*level || (newlevel = atoi(level)) <= 0) {
		SendMsgToChar("Это не похоже на уровень.\r\n", ch);
		return;
	}
	if (newlevel > kLvlImplementator) {
		sprintf(buf, "%d - максимальный возможный уровень.\r\n", kLvlImplementator);
		SendMsgToChar(buf, ch);
		return;
	}
	if (newlevel > GetRealLevel(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		SendMsgToChar("Вы не можете установить уровень выше собственного.\r\n", ch);
		return;
	}
	if (newlevel == GetRealLevel(victim)) {
		act("$E и так этого уровня.", false, ch, nullptr, victim, kToChar);
		return;
	}
	oldlevel = GetRealLevel(victim);
	if (newlevel < oldlevel) {
		SendMsgToChar("Вас окутало облако тьмы.\r\n" "Вы почувствовали себя лишенным чего-то.\r\n", victim);
	} else {
		act("$n сделал$g несколько странных пасов.\r\n"
			"Вам показалось, будто неземное тепло разлилось по каждой клеточке\r\n"
			"Вашего тела, наполняя его доселе невиданными вами ощущениями.\r\n",
			false, ch, nullptr, victim, kToVict);
	}

	SendMsgToChar(OK, ch);
	if (newlevel < oldlevel) {
		log("(GC) %s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
		imm_log("%s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
	} else {
		log("(GC) %s has advanced %s to level %d (from %d)",
			GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
		imm_log("%s has advanced %s to level %d (from %d)", GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
	}

	gain_exp_regardless(victim, GetExpUntilNextLvl(victim, newlevel)
		- GET_EXP(victim));
	victim->save_char();
}

void do_restore(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
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

		GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
		GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
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
	}
}

void perform_immort_vis(CharData *ch) {
	if (GET_INVIS_LEV(ch) == 0 &&
		!AFF_FLAGGED(ch, EAffect::kHide) && !AFF_FLAGGED(ch, EAffect::kInvisible)
		&& !AFF_FLAGGED(ch, EAffect::kDisguise)) {
		SendMsgToChar("Ну вот вас и заметили. Стало ли вам легче от этого?\r\n", ch);
		return;
	}

	SET_INVIS_LEV(ch, 0);
	appear(ch);
	SendMsgToChar("Вы теперь полностью видны.\r\n", ch);
}

void perform_immort_invis(CharData *ch, int level) {
	if (ch->IsNpc()) {
		return;
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (tch == ch) {
			continue;
		}

		if (GetRealLevel(tch) >= GET_INVIS_LEV(ch) && GetRealLevel(tch) < level) {
			act("Вы вздрогнули, когда $n растворил$u на ваших глазах.",
				false, ch, nullptr, tch, kToVict);
		}

		if (GetRealLevel(tch) < GET_INVIS_LEV(ch) && GetRealLevel(tch) >= level) {
			act("$n медленно появил$u из пустоты.",
				false, ch, nullptr, tch, kToVict);
		}
	}

	SET_INVIS_LEV(ch, level);
	sprintf(buf, "Ваш уровень невидимости - %d.\r\n", level);
	SendMsgToChar(buf, ch);
}

void do_invis(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int level;

	if (ch->IsNpc()) {
		SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		if (GET_INVIS_LEV(ch) > 0)
			perform_immort_vis(ch);
		else {
			if (GetRealLevel(ch) < kLvlImplementator)
				perform_immort_invis(ch, kLvlImmortal);
			else
				perform_immort_invis(ch, GetRealLevel(ch));
		}
	} else {
		level = MIN(atoi(arg), kLvlImplementator);
		if (level > GetRealLevel(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))
			SendMsgToChar("Вы не можете достичь невидимости выше вашего уровня.\r\n", ch);
		else if (GetRealLevel(ch) < kLvlImplementator && level > kLvlImmortal && !PRF_FLAGGED(ch, EPrf::kCoderinfo))
			perform_immort_invis(ch, kLvlImmortal);
		else if (level < 1)
			perform_immort_vis(ch);
		else
			perform_immort_invis(ch, level);
	}
}

void do_gecho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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

		if (PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
			SendMsgToChar(OK, ch);
		} else {
			SendMsgToChar(buf, ch);
		}
	}
}

void do_poofset(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
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

void do_dc(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	int num_to_dc;
	one_argument(argument, arg);
	if (!(num_to_dc = atoi(arg))) {
		SendMsgToChar("Usage: DC <user number> (type USERS for a list)\r\n", ch);
		return;
	}
	for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

	if (!d) {
		SendMsgToChar("Нет такого соединения.\r\n", ch);
		return;
	}

	if (d->character) //Чтоб не крешило при попытке разъединить незалогиненного
	{
		int victim_level = PRF_FLAGGED(d->character, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(d->character);
		int god_level = PRF_FLAGGED(ch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		if (victim_level >= god_level) {
			if (!CAN_SEE(ch, d->character))
				SendMsgToChar("Нет такого соединения.\r\n", ch);
			else
				SendMsgToChar("Да уж.. Это не есть праффильная идея...\r\n", ch);
			return;
		}
	}

	/* We used to just close the socket here using close_socket(), but
	 * various people pointed out this could cause a crash if you're
	 * closing the person below you on the descriptor list.  Just setting
	 * to CON_CLOSE leaves things in a massively inconsistent state so I
	 * had to add this new flag to the descriptor.
	 *
	 * It is a much more logical extension for a CON_DISCONNECT to be used
	 * for in-game socket closes and CON_CLOSE for out of game closings.
	 * This will retain the stability of the close_me hack while being
	 * neater in appearance. -gg 12/1/97
	 */
	if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
		SendMsgToChar("Соединение уже разорвано.\r\n", ch);
	else {
		/*
		 * Remember that we can disconnect people not in the game and
		 * that rather confuses the code when it expected there to be
		 * a character context.
		 */
		if (STATE(d) == CON_PLAYING)
			STATE(d) = CON_DISCONNECT;
		else
			STATE(d) = CON_CLOSE;

		sprintf(buf, "Соединение #%d закрыто.\r\n", num_to_dc);
		SendMsgToChar(buf, ch);
		imm_log("Connect closed by %s.", GET_NAME(ch));
	}
}

void do_wizlock(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int value;
	const char *when;

	one_argument(argument, arg);
	if (*arg) {
		value = atoi(arg);
		if (value > kLvlImplementator)
			value = kLvlImplementator; // 34е всегда должны иметь возможность зайти
		if (value < 0 || (value > GetRealLevel(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
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

void do_date(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	time_t mytime;
	std::ostringstream out;

	if (subcmd == SCMD_DATE) {
		mytime = time(nullptr);
		out << "Текущее время сервера: " << asctime(localtime(&mytime)) << std::endl;
	} else {
		mytime = shutdown_parameters.get_boot_time();
		out << " Up since: " << asctime(localtime(&mytime));
		out.seekp(-1, std::ios_base::end); // Удаляем \0 из конца строки.
		out << " ";
		PrintUptime(out);
	}

	SendMsgToChar(out.str(), ch);
}

void do_last(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Кого вы хотите найти?\r\n", ch);
		return;
	}

	Player t_chdata;
	Player *chdata = &t_chdata;
	if (load_char(arg, chdata) < 0) {
		SendMsgToChar("Нет такого игрока.\r\n", ch);
		return;
	}
	if (GetRealLevel(chdata) > GetRealLevel(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		SendMsgToChar("Вы не столь уж и божественны для этого.\r\n", ch);
	} else {
		time_t tmp_time = LAST_LOGON(chdata);
		sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
				GET_IDNUM(chdata), GetRealLevel(chdata),
				MUD::Classes(chdata->GetClass()).GetAbbr().c_str(), GET_NAME(chdata),
				GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown", ctime(&tmp_time));
		SendMsgToChar(buf, ch);
	}
}

void do_force(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i, *next_desc;
	char to_force[kMaxInputLength + 2];

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n принудил$g вас '%s'.", to_force);

	if (!*arg || !*to_force) {
		SendMsgToChar("Кого и что вы хотите принудить сделать?\r\n", ch);
	} else if (!IS_GRGOD(ch) || (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg)
		&& str_cmp("здесь", arg))) {
		const auto vict = get_char_vis(ch, arg, EFind::kCharInWorld);
		if (!vict) {
			SendMsgToChar(NOPERSON, ch);
		} else if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
			SendMsgToChar("Господи, только не это!\r\n", ch);
		} else {
			char *pstr;
			SendMsgToChar(OK, ch);
			act(buf1, true, ch, nullptr, vict, kToVict);
			sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			while ((pstr = strchr(buf, '%')) != nullptr) {
				pstr[0] = '*';
			}
			mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			imm_log("%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			command_interpreter(vict, to_force);
		}
	} else if (!str_cmp("room", arg)
		|| !str_cmp("здесь", arg)) {
		SendMsgToChar(OK, ch);
		sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (!vict->IsNpc()
				&& GetRealLevel(vict) >= GetRealLevel(ch)
				&& !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				continue;
			}

			act(buf1, true, ch, nullptr, vict, kToVict);
			command_interpreter(vict, to_force);
		}
	} else        // force all
	{
		SendMsgToChar(OK, ch);
		sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced all to %s", GET_NAME(ch), to_force);

		for (i = descriptor_list; i; i = next_desc) {
			next_desc = i->next;

			const auto vict = i->character;
			if (STATE(i) != CON_PLAYING
				|| !vict
				|| (!vict->IsNpc() && GetRealLevel(vict) >= GetRealLevel(ch)
					&& !PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
				continue;
			}

			act(buf1, true, ch, nullptr, vict.get(), kToVict);
			command_interpreter(vict.get(), to_force);
		}
	}
}

void do_sdemigod(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
				if ((!PLR_FLAGGED(d->character, EPlrFlag::kWriting)) &&
					(!PLR_FLAGGED(d->character, EPlrFlag::kMailing)) &&
					(!PLR_FLAGGED(d->character, EPrf::kDemigodChat))) {
					d->character->remember_add(buf1, Remember::ALL);
					SendMsgToChar(buf1, d->character.get());
				}
			}
		}
	}
}

void do_wiznet(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	char emote = false;
	char bookmark1 = false;
	char bookmark2 = false;
	int level = kLvlGod;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument) {
		SendMsgToChar
			("Формат: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
			 "        wiznet @<level> *<emotetext> | wiz @\r\n", ch);
		return;
	}

	//	if (EPrf::FLAGGED(ch, EPrf::CODERINFO)) return;

	// Опускаем level для gf_demigod
	if (GET_GOD_FLAG(ch, EGf::kDemigod))
		level = kLvlImmortal;

	// использование доп. аргументов
	switch (*argument) {
		case '*': emote = true;
			break;
		case '#':
			// Установить уровень имм канала
			one_argument(argument + 1, buf1);
			if (is_number(buf1)) {
				half_chop(argument + 1, buf1, argument);
				level = MAX(atoi(buf1), kLvlImmortal);
				if (level > GetRealLevel(ch)) {
					SendMsgToChar("Вы не можете изрекать выше вашего уровня.\r\n", ch);
					return;
				}
			} else if (emote)
				argument++;
			break;
		case '@':
			// Обнаруживаем всех кто может (теоретически) нас услышать
			for (d = descriptor_list; d; d = d->next) {
				if (STATE(d) == CON_PLAYING &&
					(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, EGf::kDemigod)) &&
					!PRF_FLAGGED(d->character, EPrf::kNoWiz) && (CAN_SEE(ch, d->character) || IS_IMPL(ch))) {
					if (!bookmark1) {
						strcpy(buf1,
							   "Боги/привилегированные которые смогут (наверное) вас услышать:\r\n");
						bookmark1 = true;
					}
					sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
					if (PLR_FLAGGED(d->character, EPlrFlag::kWriting))
						strcat(buf1, " (пишет)\r\n");
					else if (PLR_FLAGGED(d->character, EPlrFlag::kMailing))
						strcat(buf1, " (пишет письмо)\r\n");
					else
						strcat(buf1, "\r\n");
				}
			}
			for (d = descriptor_list; d; d = d->next) {
				if (STATE(d) == CON_PLAYING &&
					(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, EGf::kDemigod)) &&
					PRF_FLAGGED(d->character, EPrf::kNoWiz) && CAN_SEE(ch, d->character)) {
					if (!bookmark2) {
						if (!bookmark1)
							strcpy(buf1,
								   "Боги/привилегированные которые не смогут вас услышать:\r\n");
						else
							strcat(buf1,
								   "Боги/привилегированные которые не смогут вас услышать:\r\n");

						bookmark2 = true;
					}
					sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
				}
			}
			SendMsgToChar(buf1, ch);

			return;
		case '\\': ++argument;
			break;
		default: break;
	}
	if (PRF_FLAGGED(ch, EPrf::kNoWiz)) {
		SendMsgToChar("Вы вне игры!\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("Не думаю, что Боги одобрят это.\r\n", ch);
		return;
	}
	if (level != kLvlGod) {
		sprintf(buf1, "%s%s: <%d> %s%s\r\n", GET_NAME(ch),
				emote ? "" : " богам", level, emote ? "<--- " : "", argument);
	} else {
		sprintf(buf1, "%s%s: %s%s\r\n", GET_NAME(ch), emote ? "" : " богам", emote ? "<--- " : "", argument);
	}
	snprintf(buf2, kMaxStringLength, "&c%s&n", buf1);
	Remember::add_to_flaged_cont(Remember::wiznet_, buf2, level);

	// пробегаемся по списку дескрипторов чаров и кто должен - тот услышит богов
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) &&    // персонаж должен быть в игре
			((GetRealLevel(d->character) >= level) ||    // уровень равным или выше level
				(GET_GOD_FLAG(d->character, EGf::kDemigod) && level == 31)) &&    // демигоды видят 31 канал
			(!PRF_FLAGGED(d->character, EPrf::kNoWiz)) &&    // игрок с режимом NOWIZ не видит имм канала
			(!PLR_FLAGGED(d->character, EPlrFlag::kWriting)) &&    // пишущий не видит имм канала
			(!PLR_FLAGGED(d->character, EPlrFlag::kMailing)))    // отправляющий письмо не видит имм канала
		{
			// отправляем сообщение чару
			snprintf(buf2, kMaxStringLength, "%s%s%s",
					 CCCYN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM));
			d->character->remember_add(buf2, Remember::ALL);
			// не видино своих мессаг если 'режим repeat'
			if (d != ch->desc
				|| !(PRF_FLAGGED(d->character, EPrf::kNoRepeat))) {
				SendMsgToChar(buf2, d->character.get());
			}
		}
	}

	if (PRF_FLAGGED(ch, EPrf::kNoRepeat)) {
		SendMsgToChar(OK, ch);
	}
}

void do_zreset(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ZoneRnum i;
	ZoneVnum j;

	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Укажите зону.\r\n", ch);
		return;
	}
	if (*arg == '*') {
		for (i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
			reset_zone(i);
		}

		SendMsgToChar("Перезагружаю мир.\r\n", ch);
		sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset entire world.", GET_NAME(ch));
		return;
	} else if (*arg == '.') {
		i = world[ch->in_room]->zone_rn;
	} else {
		j = atoi(arg);
		for (i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
			if (zone_table[i].vnum == j) {
				break;
			}
		}
	}

	if (i >= 0 && i < static_cast<ZoneRnum>(zone_table.size())) {
		reset_zone(i);
		sprintf(buf, "Перегружаю зону %d (#%d): %s.\r\n", i, zone_table[i].vnum, zone_table[i].name);
		SendMsgToChar(buf, ch);
		sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
	} else {
		SendMsgToChar("Нет такой зоны.\r\n", ch);
	}
}

// Функции установки разных наказаний.

// *  General fn for wizcommands of the sort: cmd <player>
void do_wizutil(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	long result;
	int times = 0;
	char *reason;
	char num[kMaxInputLength];

	//  one_argument(argument, arg);
	reason = two_arguments(argument, arg, num);

	if (!*arg)
		SendMsgToChar("Для кого?\r\n", ch);
	else if (!(vict = get_player_pun(ch, arg, EFind::kCharInWorld)))
		SendMsgToChar("Нет такого игрока.\r\n", ch);
	else if (GetRealLevel(vict) > GetRealLevel(ch) && !GET_GOD_FLAG(ch, EGf::kDemigod) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))
		SendMsgToChar("А он ведь старше вас....\r\n", ch);
	else if (GetRealLevel(vict) >= kLvlImmortal && GET_GOD_FLAG(ch, EGf::kDemigod))
		SendMsgToChar("А он ведь старше вас....\r\n", ch);
	else {
		switch (subcmd) {
			case SCMD_REROLL: SendMsgToChar("Перегенерирую...\r\n", ch);
				roll_real_abils(vict);
				log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
				imm_log("%s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
				sprintf(buf,
						"Новые параметры: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
						vict->GetInbornStr(), vict->GetInbornInt(), vict->GetInbornWis(),
						vict->GetInbornDex(), vict->GetInbornCon(), vict->GetInbornCha());
				SendMsgToChar(buf, ch);
				break;
			case SCMD_NOTITLE: result = PLR_TOG_CHK(vict, EPlrFlag::kNoTitle);
				sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
				mudlog(buf, NRM, MAX(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
				strcat(buf, "\r\n");
				SendMsgToChar(buf, ch);
				break;
			case SCMD_SQUELCH: break;
			case SCMD_MUTE: if (*num) times = atol(num);
				set_punish(ch, vict, SCMD_MUTE, reason, times);
				break;
			case SCMD_DUMB: if (*num) times = atol(num);
				set_punish(ch, vict, SCMD_DUMB, reason, times);
				break;
			case SCMD_FREEZE: if (*num) times = atol(num);
				set_punish(ch, vict, SCMD_FREEZE, reason, times);
				break;
			case SCMD_HELL: if (*num) times = atol(num);
				set_punish(ch, vict, SCMD_HELL, reason, times);
				break;

			case SCMD_NAME: if (*num) times = atol(num);
				set_punish(ch, vict, SCMD_NAME, reason, times);
				break;

			case SCMD_REGISTER: set_punish(ch, vict, SCMD_REGISTER, reason, 0);
				break;

			case SCMD_UNREGISTER: set_punish(ch, vict, SCMD_UNREGISTER, reason, 0);
				break;

			case SCMD_UNAFFECT:
				if (!vict->affected.empty()) {
					while (!vict->affected.empty()) {
						vict->affect_remove(vict->affected.begin());
					}
					SendMsgToChar("Яркая вспышка осветила вас!\r\n"
								  "Вы почувствовали себя немного иначе.\r\n", vict);
					SendMsgToChar("Все афекты сняты.\r\n", ch);
				} else {
					SendMsgToChar("Аффектов не было изначально.\r\n", ch);
					return;
				}
				break;
			default: log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
				break;
		}
		vict->save_char();
	}
}

void show_apply(CharData *ch, CharData *vict) {
	ObjData *obj = nullptr;
	for (int i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if ((obj = GET_EQ(vict, i))) {
			SendMsgToChar(ch, "Предмет: %s (%d)\r\n", GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
			// Update weapon applies
			for (int j = 0; j < kMaxObjAffect; j++) {
				if (GET_EQ(vict, i)->get_affected(j).modifier != 0) {
						SendMsgToChar(ch, "Добавляет (apply): %s, модификатор: %d\r\n",
							apply_types[(int) GET_EQ(vict, i)->get_affected(j).location], GET_EQ(vict, i)->get_affected(j).modifier);
				}
			}
		}
	}
}

// **************** The do_set function

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC    0
#define BINARY    1
#define NUMBER    2

inline void SET_OR_REMOVE(const bool on, const bool off, FlagData &flagset, const Bitvector packed_flag) {
	if (on) {
		flagset.set(packed_flag);
	} else if (off) {
		flagset.unset(packed_flag);
	}
}

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

// The set options available
struct set_struct        /*
				   { const char *cmd;
				   const char level;
				   const char pcnpc;
				   const char type;
				   } */ set_fields[] =
	{
		{"brief", kLvlGod, PC, BINARY},    // 0
		{"invstart", kLvlGod, PC, BINARY},    // 1
		{"nosummon", kLvlGreatGod, PC, BINARY},
		{"maxhit", kLvlImplementator, BOTH, NUMBER},
		{"maxmana", kLvlGreatGod, BOTH, NUMBER},
		{"maxmove", kLvlImplementator, BOTH, NUMBER},    // 5
		{"hit", kLvlGreatGod, BOTH, NUMBER},
		{"mana", kLvlGreatGod, BOTH, NUMBER},
		{"move", kLvlGreatGod, BOTH, NUMBER},
		{"race", kLvlGreatGod, BOTH, NUMBER},
		{"size", kLvlImplementator, BOTH, NUMBER},    // 10
		{"ac", kLvlGreatGod, BOTH, NUMBER},
		{"gold", kLvlImplementator, BOTH, NUMBER},
		{"bank", kLvlImplementator, PC, NUMBER},
		{"exp", kLvlImplementator, BOTH, NUMBER},
		{"hitroll", kLvlImplementator, BOTH, NUMBER}, // 15
		{"damroll", kLvlImplementator, BOTH, NUMBER},
		{"invis", kLvlImplementator, PC, NUMBER},
		{"nohassle", kLvlImplementator, PC, BINARY},
		{"frozen", kLvlGreatGod, PC, MISC},
		{"practices", kLvlGreatGod, PC, NUMBER}, // 20
		{"lessons", kLvlGreatGod, PC, NUMBER},
		{"drunk", kLvlGreatGod, BOTH, MISC},
		{"hunger", kLvlGreatGod, BOTH, MISC},
		{"thirst", kLvlGreatGod, BOTH, MISC},
		{"thief", kLvlGod, PC, BINARY}, // 25
		{"level", kLvlImplementator, BOTH, NUMBER},
		{"room", kLvlImplementator, BOTH, NUMBER},
		{"roomflag", kLvlGreatGod, PC, BINARY},
		{"siteok", kLvlGreatGod, PC, BINARY},
		{"deleted", kLvlImplementator, PC, BINARY}, // 30
		{"class", kLvlImplementator, BOTH, MISC},
		{"demigod", kLvlImplementator, PC, BINARY},
		{"loadroom", kLvlGreatGod, PC, MISC},
		{"color", kLvlGod, PC, BINARY},
		{"idnum", kLvlImplementator, PC, NUMBER}, // 35
		{"passwd", kLvlImplementator, PC, MISC},
		{"nodelete", kLvlGod, PC, BINARY},
		{"sex", kLvlGreatGod, BOTH, MISC},
		{"age", kLvlGreatGod, BOTH, NUMBER},
		{"height", kLvlGod, BOTH, NUMBER}, // 40
		{"weight", kLvlGod, BOTH, NUMBER},
		{"godslike", kLvlImplementator, BOTH, BINARY},
		{"godscurse", kLvlImplementator, BOTH, BINARY},
		{"olc", kLvlImplementator, PC, NUMBER},
		{"name", kLvlGreatGod, PC, MISC}, // 45
		{"trgquest", kLvlImplementator, PC, MISC},
		{"mkill", kLvlImplementator, PC, MISC},
		{"highgod", kLvlImplementator, PC, MISC},
		{"hell", kLvlGod, PC, MISC},
		{"email", kLvlGod, PC, MISC}, //50
		{"religion", kLvlGod, PC, MISC},
		{"perslog", kLvlImplementator, PC, BINARY},
		{"mute", kLvlGod, PC, MISC},
		{"dumb", kLvlGod, PC, MISC},
		{"karma", kLvlImplementator, PC, MISC},
		{"unreg", kLvlGod, PC, MISC}, // 56
		{"палач", kLvlImplementator, PC, BINARY}, // 57
		{"killer", kLvlImplementator, PC, BINARY}, // 58
		{"remort", kLvlImplementator, PC, NUMBER}, // 59
		{"tester", kLvlImplementator, PC, BINARY}, // 60
		{"autobot", kLvlImplementator, PC, BINARY}, // 61
		{"hryvn", kLvlImplementator, PC, NUMBER}, // 62
		{"scriptwriter", kLvlImplementator, PC, BINARY}, // 63
		{"spammer", kLvlGod, PC, BINARY}, // 64
		{"gloryhide", kLvlImplementator, PC, BINARY}, // 65
		{"telegram", kLvlImplementator, PC, MISC}, // 66
		{"nogata", kLvlImplementator, PC, NUMBER}, // 67
		{"\n", 0, BOTH, MISC}
	};

int perform_set(CharData *ch, CharData *vict, int mode, char *val_arg) {
	int i, j, c, value = 0, return_code = 1, ptnum, times = 0;
	bool on = false;
	bool off = false;
	char npad[ECase::kLastCase + 1][256];
	char *reason;
	RoomRnum rnum;
	RoomVnum rvnum;
	char output[kMaxStringLength], num[kMaxInputLength];
	int rod;

	// Check to make sure all the levels are correct
	if (!IS_IMPL(ch)) {
		if (!vict->IsNpc() && vict != ch) {
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if (GetRealLevel(ch) <= GetRealLevel(vict) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
					SendMsgToChar("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			} else {
				if (GetRealLevel(vict) >= kLvlImmortal || PRF_FLAGGED(vict, EPrf::kCoderinfo)) {
					SendMsgToChar("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			}
		}
	}
	if (!privilege::HasPrivilege(ch, std::string(set_fields[mode].cmd), 0, 1)) {
		SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
		return (0);
	}

	// Make sure the PC/NPC is correct
	if (vict->IsNpc() && !(set_fields[mode].pcnpc & NPC)) {
		SendMsgToChar("Эта тварь недостойна такой чести!\r\n", ch);
		return (0);
	} else if (!vict->IsNpc() && !(set_fields[mode].pcnpc & PC)) {
		act("Вы оскорбляете $S - $E ведь не моб!", false, ch, nullptr, vict, kToChar);
		return (0);
	}

	// Find the value of the argument
	if (set_fields[mode].type == BINARY) {
		if (!strn_cmp(val_arg, "on", 2) || !strn_cmp(val_arg, "yes", 3) || !strn_cmp(val_arg, "вкл", 3))
			on = true;
		else if (!strn_cmp(val_arg, "off", 3) || !strn_cmp(val_arg, "no", 2) || !strn_cmp(val_arg, "выкл", 4))
			off = true;
		if (!(on || off)) {
			SendMsgToChar("Значение может быть 'on' или 'off'.\r\n", ch);
			return (0);
		}
		sprintf(output, "%s %s для %s.", set_fields[mode].cmd, ONOFF(on), GET_PAD(vict, 1));
	} else if (set_fields[mode].type == NUMBER) {
		value = atoi(val_arg);
		sprintf(output, "У %s %s установлено в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
	} else {
		strcpy(output, "Хорошо.");
	}
	switch (mode) {
		case 0: SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::kBrief);
			break;
		case 1: SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kInvStart);
			break;
		case 2: SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::KSummonable);
			sprintf(output, "Возможность призыва %s для %s.\r\n", ONOFF(!on), GET_PAD(vict, 1));
			break;
		case 3: vict->points.max_hit = RANGE(1, 5000);
			affect_total(vict);
			break;
		case 4: break;
		case 5: vict->points.max_move = RANGE(1, 5000);
			affect_total(vict);
			break;
		case 6: vict->points.hit = RANGE(-9, vict->points.max_hit);
			affect_total(vict);
			break;
		case 7: break;
		case 8: break;
		case 9:
			// Выставляется род для РС
			rod = PlayerRace::CheckRace(GET_KIN(ch), val_arg);
			if (rod == RACE_UNDEFINED) {
				SendMsgToChar("Не было таких на земле русской!\r\n", ch);
				SendMsgToChar(PlayerRace::ShowRacesMenu(GET_KIN(ch)), ch);
				return (0);
			} else {
				GET_RACE(vict) = rod;
				affect_total(vict);

			}
			break;
		case 10: vict->real_abils.size = RANGE(1, 100);
			affect_total(vict);
			break;
		case 11: vict->real_abils.armor = RANGE(-100, 100);
			affect_total(vict);
			break;
		case 12: vict->set_gold(value);
			break;
		case 13: vict->set_bank(value);
			break;
		case 14:
			//vict->points.exp = RANGE(0, 7000000);
			RANGE(0, GetExpUntilNextLvl(vict, kLvlImmortal) - 1);
			gain_exp_regardless(vict, value - GET_EXP(vict));
			break;
		case 15: vict->real_abils.hitroll = RANGE(-20, 20);
			affect_total(vict);
			break;
		case 16: vict->real_abils.damroll = RANGE(-20, 20);
			affect_total(vict);
			break;
		case 17:
			if (!IS_IMPL(ch) && ch != vict && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return (0);
			}
			SET_INVIS_LEV(vict, RANGE(0, GetRealLevel(vict)));
			break;
		case 18:
			if (!IS_IMPL(ch) && ch != vict && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return (0);
			}
			SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::kNohassle);
			break;
		case 19: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!strcmp(num, "0")) {
				if (!set_punish(ch, vict, SCMD_FREEZE, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!set_punish(ch, vict, SCMD_FREEZE, reason, times))
					return (0);
			}
			break;
		case 20:
		case 21: return_code = 0;
			break;
		case 22:
		case 23:
		case 24: {
			const unsigned num = mode - 22; // magic number циркулевских времен
			if (num >= (ch)->player_specials->saved.conditions.size()) {
				SendMsgToChar("Ошибка: num >= saved.conditions.size(), сообщите кодерам.\r\n", ch);
				return 0;
			}
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				GET_COND(vict, num) = -1;
				sprintf(output, "Для %s %s сейчас отключен.", GET_PAD(vict, 1), set_fields[mode].cmd);
			} else if (is_number(val_arg)) {
				value = atoi(val_arg);
				RANGE(0, kMaxCondition);
				GET_COND(vict, num) = value;
				sprintf(output, "Для %s %s установлен в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
			} else {
				SendMsgToChar("Должно быть 'off' или значение от 0 до 24.\r\n", ch);
				return 0;
			}
			break;
		}
		case 25: SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kBurglar);
			break;
		case 26:
			if (!PRF_FLAGGED(ch, EPrf::kCoderinfo)
				&& (value > GetRealLevel(ch) || value > kLvlImplementator || GetRealLevel(vict) > GetRealLevel(ch))) {
				SendMsgToChar("Вы не можете установить уровень игрока выше собственного.\r\n", ch);
				return (0);
			}
			RANGE(0, kLvlImplementator);
			vict->set_level(value);
			break;
		case 27:
			if ((rnum = real_room(value)) == kNowhere) {
				SendMsgToChar("Поищите другой МУД. В этом МУДе нет такой комнаты.\r\n", ch);
				return (0);
			}
			if (IN_ROOM(vict) != kNowhere)    // Another Eric Green special.
				ExtractCharFromRoom(vict);
			PlaceCharToRoom(vict, rnum);
			vict->dismount();
			break;
		case 28: SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::kRoomFlags);
			break;
		case 29: SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kSiteOk);
			break;
		case 30:
			if (IS_IMPL(vict) || PRF_FLAGGED(vict, EPrf::kCoderinfo)) {
				SendMsgToChar("Истинные боги вечны!\r\n", ch);
				return 0;
			}
			SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kDeleted);
			if (PLR_FLAGS(vict).get(EPlrFlag::kDeleted)) {
				if (PLR_FLAGS(vict).get(EPlrFlag::kNoDelete)) {
					PLR_FLAGS(vict).unset(EPlrFlag::kNoDelete);
					SendMsgToChar("NODELETE flag also removed.\r\n", ch);
				}
			}
			break;
		case 31: {
			auto class_id = FindAvailableCharClassId(val_arg);
			if (class_id == ECharClass::kUndefined) {
				SendMsgToChar("Нет такого класса в этой игре. Найдите себе другую.\r\n", ch);
				return (0);
			}
			vict->set_class(class_id);
			break;
		}
		case 32:
			// Флаг для морталов с привилегиями
			if (!IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
				return 0;
			}
			if (on) {
				SET_GOD_FLAG(vict, EGf::kDemigod);
			} else if (off) {
				CLR_GOD_FLAG(vict, EGf::kDemigod);
			}
			break;
		case 33:
			if (is_number(val_arg)) {
				rvnum = atoi(val_arg);
				if (real_room(rvnum) != kNowhere) {
					GET_LOADROOM(vict) = rvnum;
					sprintf(output, "%s будет входить в игру из комнаты #%d.",
							GET_NAME(vict), GET_LOADROOM(vict));
				} else {
					SendMsgToChar
						("Прежде чем кого-то куда-то поместить, надо это КУДА-ТО создать.\r\n"
						 "Скажите Стрибогу - пусть зоны рисует, а не пьянствует.\r\n", ch);
					return (0);
				}
			} else {
				SendMsgToChar("Должен быть виртуальный номер комнаты.\r\n", ch);
				return (0);
			}
			break;
		case 34: SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::kColor1);
			SET_OR_REMOVE(on, off, PRF_FLAGS(vict), EPrf::kColor2);
			break;
		case 35:
			if (!IS_IMPL(ch) || !vict->IsNpc()) {
				return (0);
			}
			vict->set_idnum(value);
			break;
		case 36:
			if (!IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo) && ch != vict) {
				SendMsgToChar("Давайте не будем экспериментировать.\r\n", ch);
				return (0);
			}
			if (IS_IMPL(vict) && ch != vict) {
				SendMsgToChar("Вы не можете ЭТО изменить.\r\n", ch);
				return (0);
			}
			if (!Password::check_password(vict, val_arg)) {
				SendMsgToChar(ch, "%s\r\n", Password::BAD_PASSWORD);
				return 0;
			}
			Password::set_password(vict, val_arg);
			Password::send_password(GET_EMAIL(vict), val_arg, std::string(GET_NAME(vict)));
			sprintf(buf, "%s заменен пароль богом.", GET_PAD(vict, 2));
			add_karma(vict, buf, GET_NAME(ch));
			sprintf(output, "Пароль изменен на '%s'.", val_arg);
			break;
		case 37: SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kNoDelete);
			break;
		case 38:
			if ((i = search_block(val_arg, genders, false)) < 0) {
				SendMsgToChar
					("Может быть 'мужчина', 'женщина', или 'бесполое'(а вот это я еще не оценил :).\r\n", ch);
				return (0);
			}
			vict->set_sex(static_cast<ESex>(i));
			break;

		case 39:        // set age
			if (value < 2 || value > 200)    // Arbitrary limits.
			{
				SendMsgToChar("Поддерживаются возрасты от 2 до 200.\r\n", ch);
				return (0);
			}
			/*
		 * NOTE: May not display the exact age specified due to the integer
		 * division used elsewhere in the code.  Seems to only happen for
		 * some values below the starting age (17) anyway. -gg 5/27/98
		 */
			vict->player_data.time.birth = time(nullptr) - ((value - 17) * kSecsPerMudYear);
			break;

		case 40:        // Blame/Thank Rick Glover. :)
			GET_HEIGHT(vict) = value;
			affect_total(vict);
			break;

		case 41: GET_WEIGHT(vict) = value;
			affect_total(vict);
			break;

		case 42:
			if (on) {
				SET_GOD_FLAG(vict, EGf::kGodsLike);
				if (sscanf(val_arg, "%s %d", npad[0], &i) != 0)
					GCURSE_DURATION(vict) = (i > 0) ? time(nullptr) + i * 60 * 60 : MAX_TIME;
				else
					GCURSE_DURATION(vict) = 0;
				sprintf(buf, "%s установил GUDSLIKE персонажу %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImplementator, SYSLOG, 0);

			} else if (off)
				CLR_GOD_FLAG(vict, EGf::kGodsLike);
			break;
		case 43:
			if (on) {
				SET_GOD_FLAG(vict, EGf::kGodscurse);
				if (sscanf(val_arg, "%s %d", npad[0], &i) != 0)
					GCURSE_DURATION(vict) = (i > 0) ? time(nullptr) + i * 60 * 60 : MAX_TIME;
				else
					GCURSE_DURATION(vict) = 0;
			} else if (off)
				CLR_GOD_FLAG(vict, EGf::kGodscurse);
			break;
		case 44:
			if (PRF_FLAGGED(ch, EPrf::kCoderinfo) || IS_IMPL(ch))
				GET_OLC_ZONE(vict) = value;
			else {
				sprintf(buf, "Слишком низкий уровень чтоб раздавать права OLC.\r\n");
				SendMsgToChar(buf, ch);
			}
			break;

		case 45:
			// изменение имени !!!

			if ((i = sscanf(val_arg, "%s %s %s %s %s %s", npad[0], npad[1], npad[2], npad[3], npad[4], npad[5])) != 6) {
				sprintf(buf, "Требуется указать 6 падежей, найдено %d\r\n", i);
				SendMsgToChar(buf, ch);
				return (0);
			}

			if (*npad[0] == '*')    // Only change pads
			{
				for (i = ECase::kGen; i <= ECase::kLastCase; i++)
					if (!_parse_name(npad[i], npad[i])) {
						vict->player_data.PNames[i] = std::string(npad[i]);
					}
				sprintf(buf, "Произведена замена падежей.\r\n");
				SendMsgToChar(buf, ch);
			} else {
				for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
					if (strlen(npad[i]) < kMinNameLength || strlen(npad[i]) > kMaxNameLength) {
						sprintf(buf, "Падеж номер %d некорректен.\r\n", ++i);
						SendMsgToChar(buf, ch);
						return (0);
					}
				}

				if (_parse_name(npad[0], npad[0]) ||
					strlen(npad[0]) < kMinNameLength ||
					strlen(npad[0]) > kMaxNameLength ||
					!Valid_Name(npad[0]) || reserved_word(npad[0]) || fill_word(npad[0])) {
					SendMsgToChar("Некорректное имя.\r\n", ch);
					return (0);
				}

				if ((get_id_by_name(npad[0]) >= 0) && !PLR_FLAGS(vict).get(EPlrFlag::kDeleted)) {
					SendMsgToChar("Это имя совпадает с именем другого персонажа.\r\n"
								  "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
					return (0);
				}
				// выносим из листа неодобренных имен, если есть
				NewNames::remove(vict);

				ptnum = get_ptable_by_name(GET_NAME(vict));
				if (ptnum < 0)
					return (0);

				if (!PLR_FLAGS(vict).get(EPlrFlag::kFrozen)
					&& !PLR_FLAGS(vict).get(EPlrFlag::kDeleted)
					&& !IS_IMMORTAL(vict)) {
					TopPlayer::Remove(vict);
				}

				for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
					if (!_parse_name(npad[i], npad[i])) {
						vict->player_data.PNames[i] = std::string(npad[i]);
					}
				}
				sprintf(buf, "Name changed from %s to %s", GET_NAME(vict), npad[0]);
				vict->set_name(npad[0]);
				add_karma(vict, buf, GET_NAME(ch));

				if (!PLR_FLAGS(vict).get(EPlrFlag::kFrozen)
					&& !PLR_FLAGS(vict).get(EPlrFlag::kDeleted)
					&& !IS_IMMORTAL(vict)) {
					TopPlayer::Refresh(vict);
				}

				player_table.set_name(ptnum, npad[0]);

				return_code = 2;
				PLR_FLAGS(vict).set(EPlrFlag::kCrashSave);
			}
			break;

		case 46:

			npad[1][0] = '\0';
			if (sscanf(val_arg, "%d %s %[^\n]", &ptnum, npad[0], npad[1]) != 3) {
				if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2) {
					SendMsgToChar("Формат : set <имя> trgquest <quest_num> <on|off> <строка данных>\r\n", ch);
					return 0;
				}
			}

			if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл")) {
				if (!vict->quested_remove(ptnum)) {
					act("$N не выполнял$G этого квеста.", false, ch, nullptr, vict, kToChar);
					return 0;
				}
			} else if (!str_cmp(npad[0], "on") || !str_cmp(npad[0], "вкл")) {
				vict->quested_add(vict, ptnum, npad[1]);
			} else {
				SendMsgToChar("Требуется on или off.\r\n", ch);
				return 0;
			}
			break;

		case 47:

			if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2) {
				SendMsgToChar("Формат : set <имя> mkill <MobVnum> <off|num>\r\n", ch);
				return (0);
			}
			if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл"))
				vict->mobmax_remove(ptnum);
			else if ((j = atoi(npad[0])) > 0) {
				if ((c = vict->mobmax_get(ptnum)) != j)
					vict->mobmax_add(vict, ptnum, j - c, MobMax::get_level_by_vnum(ptnum));
				else {
					act("$N убил$G именно столько этих мобов.", false, ch, nullptr, vict, kToChar);
					return (0);
				}
			} else {
				SendMsgToChar("Требуется off или значение больше 0.\r\n", ch);
				return (0);
			}
			break;

		case 48: return (0);
			break;

		case 49: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!set_punish(ch, vict, SCMD_HELL, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!set_punish(ch, vict, SCMD_HELL, reason, times))
					return (0);
			}
			break;
		case 50:
			if (IsValidEmail(val_arg)) {
				utils::ConvertToLow(val_arg);
				sprintf(buf, "Email changed from %s to %s", GET_EMAIL(vict), val_arg);
				add_karma(vict, buf, GET_NAME(ch));
				strncpy(GET_EMAIL(vict), val_arg, 127);
				*(GET_EMAIL(vict) + 127) = '\0';
			} else {
				SendMsgToChar("Wrong E-Mail.\r\n", ch);
				return (0);
			}
			break;

		case 51:
			// Выставляется род для РС
			rod = (*val_arg);
			if (rod != '0' && rod != '1') {
				SendMsgToChar("Не было таких на земле русской!\r\n", ch);
				SendMsgToChar("0 - Язычество, 1 - Христианство\r\n", ch);
				return (0);
			} else {
				GET_RELIGION(vict) = rod - '0';
			}
			break;

		case 52:
			// Отдельный лог команд персонажа
			if (on) {
				SET_GOD_FLAG(vict, EGf::kPerslog);
			} else if (off) {
				CLR_GOD_FLAG(vict, EGf::kPerslog);
			}
			break;

		case 53: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!set_punish(ch, vict, SCMD_MUTE, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!set_punish(ch, vict, SCMD_MUTE, reason, times))
					return (0);
			}
			break;

		case 54: reason = one_argument(val_arg, num);
			if (!*num) {
				SendMsgToChar(ch, "Укажите срок наказания.\r\n");
				return (0); // return(0) обходит запись в файл в do_set
			}
			if (!str_cmp(num, "0")) {
				if (!set_punish(ch, vict, SCMD_DUMB, reason, times)) {
					return (0);
				}
				return (1);
			}
			times = atol(num);
			if (times == 0) {
				SendMsgToChar(ch, "Срок указан не верно.\r\n");
				return (0);
			} else {
				if (!set_punish(ch, vict, SCMD_DUMB, reason, times))
					return (0);
			}
			break;
		case 55:
			if (GetRealLevel(vict) >= kLvlImmortal && !IS_IMPL(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
				SendMsgToChar("Кем вы себя возомнили?\r\n", ch);
				return 0;
			}
			reason = strdup(val_arg);
			if (reason && *reason) {
				skip_spaces(&reason);
				sprintf(buf, "add by %s", GET_NAME(ch));
				if (!strcmp(reason, "clear")) {
					if KARMA(vict)
						free(KARMA(vict));

					KARMA(vict) = nullptr;
					act("Вы отпустили $N2 все грехи.", false, ch, nullptr, vict, kToChar);
					sprintf(buf, "%s", GET_NAME(ch));
					add_karma(vict, "Очистка грехов", buf);

				} else add_karma(vict, buf, reason);
			} else {
				SendMsgToChar("Формат команды: set [ file | player ] <character> karma <reason>\r\n", ch);
				return (0);
			}
			break;

		case 56:      // Разрегистрация персонажа
			reason = one_argument(val_arg, num);
			if (!set_punish(ch, vict, SCMD_UNREGISTER, reason, 0)) return (0);
			break;

		case 57:      // Установка флага палач
			reason = one_argument(val_arg, num);
			skip_spaces(&reason);
			sprintf(buf, "executor %s by %s", (on ? "on" : "off"), GET_NAME(ch));
//			add_karma(vict, buf, reason);
			if (on) {
				PRF_FLAGS(vict).set(EPrf::kExecutor);
			} else if (off) {
				PRF_FLAGS(vict).unset(EPrf::kExecutor);
			}
			break;

		case 58: // Снятие или постановка флага !ДУШЕГУБ! только для имплементоров
			SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kKiller);
			break;
		case 59: // флаг реморта
			if (value > 1 && value < 75) {
				sprintf(buf, "Иммортал %s установил реморт %d  для игрока %s ", GET_NAME(ch), value, GET_NAME(vict));
				add_karma(vict, buf, GET_NAME(ch));
				add_karma(ch, buf, GET_NAME(vict));
				vict->set_remort(value);
				SendMsgToGods(buf);
			}
			else {
				SendMsgToChar(ch, "Не правильно указан реморт.\r\n");
			}
			break;
		case 60: // флаг тестера
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				CLR_GOD_FLAG(vict, EGf::kAllowTesterMode);
				PRF_FLAGS(vict).unset(EPrf::kTester); // обнулим реж тестер
				sprintf(buf, "%s убрал флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
			} else {
				SET_GOD_FLAG(vict, EGf::kAllowTesterMode);
				sprintf(buf, "%s установил флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				//			send_to_gods(buf);
			}
			break;
		case 61: // флаг автобота
		{
			SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kAutobot);
			break;
		}
		case 62: vict->set_hryvn(value);
			break;
		case 63: // флаг скриптера
			sprintf(buf, "%s", GET_NAME(ch));
			if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл")) {
				PLR_FLAGS(vict).unset(EPlrFlag::kScriptWriter);
				add_karma(vict, "Снятие флага скриптера", buf);
				sprintf(buf, "%s убрал флаг скриптера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				return (1);
			} else if (!str_cmp(val_arg, "on") || !str_cmp(val_arg, "вкл")) {
				PLR_FLAGS(vict).set(EPlrFlag::kScriptWriter);
				add_karma(vict, "Установка флага скриптера", buf);
				sprintf(buf, "%s установил  флаг скриптера для игрока %s", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
				return (1);
			} else {
				SendMsgToChar(ch, "Значение может быть только on/off или вкл/выкл.\r\n");
				return (0); // не пишем в пфайл ничего
			}
			break;
		case 64: // флаг спамера
		{
			SET_OR_REMOVE(on, off, PLR_FLAGS(vict), EPlrFlag::kSpamer);
			break;
		}
		case 65: { // спрятать отображение славы
			if (!str_cmp(val_arg, "on") || !str_cmp(val_arg, "вкл")) {
				GloryConst::glory_hide(vict, true);
			} else {
				GloryConst::glory_hide(vict, false);
			}
			break;
		}
		case 66: { // идентификатор чата телеграма

			unsigned long int id = strtoul(val_arg, nullptr, 10);
			if (!ch->IsNpc() && id != 0) {
				sprintf(buf, "Telegram chat_id изменен с %lu на %lu\r\n", vict->player_specials->saved.telegram_id, id);
				SendMsgToChar(buf, ch);
				vict->setTelegramId(id);
			} else
				SendMsgToChar("Ошибка, указано неверное число или персонаж.\r\n", ch);
			break;
		}
		case 67: vict->set_nogata(value);
			break;
		default: SendMsgToChar("Не могу установить это!\r\n", ch);
			return (0);
	}

	strcat(output, "\r\n");
	SendMsgToChar(CAP(output), ch);
	return (return_code);
}

void do_set(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;
	char field[kMaxInputLength], name[kMaxInputLength], val_arg[kMaxInputLength], OName[kMaxInputLength];
	int mode, player_i = 0, retval;
	char is_file = 0, is_player = 0;

	half_chop(argument, name, buf);

	if (!*name) {
		strcpy(buf, "Возможные поля для изменения:\r\n");
		for (int i = 0; set_fields[i].level; i++)
			if (privilege::HasPrivilege(ch, std::string(set_fields[i].cmd), 0, 1))
				sprintf(buf + strlen(buf), "%-15s%s", set_fields[i].cmd, (!((i + 1) % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		SendMsgToChar(buf, ch);
		return;
	}

	if (!strcmp(name, "file")) {
		is_file = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "player")) {
		is_player = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "mob")) {
		half_chop(buf, name, buf);
	} else
		is_player = 1;

	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*name || !*field) {
		SendMsgToChar("Usage: set [mob|player|file] <victim> <field> <value>\r\n", ch);
		return;
	}

	CharData::shared_ptr cbuf;
	// find the target
	if (!is_file) {
		if (is_player) {

			if (!(vict = get_player_pun(ch, name, EFind::kCharInWorld))) {
				SendMsgToChar("Нет такого игрока.\r\n", ch);
				return;
			}

			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if ((GetRealLevel(ch) <= GetRealLevel(vict)) && !(is_head(ch->get_name_str()))) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			} else {
				if (GetRealLevel(vict) >= kLvlImmortal) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
		} else    // is_mob
		{
			if (!(vict = get_char_vis(ch, name, EFind::kCharInWorld))
				|| !vict->IsNpc()) {
				SendMsgToChar("Нет такой твари Божьей.\r\n", ch);
				return;
			}
		}
	} else if (is_file)    // try to load the player off disk
	{
		if (get_player_pun(ch, name, EFind::kCharInWorld)) {
			SendMsgToChar("Да разуй же глаза! Оно в сети!\r\n", ch);
			return;
		}

		cbuf = std::make_unique<Player>();
		if ((player_i = load_char(name, cbuf.get())) > -1) {
			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, EGf::kDemigod)) {
				if (GetRealLevel(ch) <= GetRealLevel(cbuf) && !(is_head(ch->get_name_str()))) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			} else {
				if (GetRealLevel(cbuf) >= kLvlImmortal) {
					SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
			vict = cbuf.get();
		} else {
			SendMsgToChar("Нет такого игрока.\r\n", ch);
			return;
		}
	}

	// find the command in the list
	size_t len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++) {
		if (!strncmp(field, set_fields[mode].cmd, len)) {
			break;
		}
	}

	// perform the set
	strcpy(OName, GET_NAME(vict));
	retval = perform_set(ch, vict, mode, val_arg);

	// save the character if a change was made
	if (retval && !vict->IsNpc()) {
		if (retval == 2) {
			rename_char(vict, OName);
		} else {
			if (!is_file && !vict->IsNpc()) {
				vict->save_char();
			}
			if (is_file) {
				vict->save_char();
				SendMsgToChar("Файл сохранен.\r\n", ch);
			}
		}
	}

	log("(GC) %s try to set: %s", GET_NAME(ch), argument);
	imm_log("%s try to set: %s", GET_NAME(ch), argument);
}

int exchange(CharData *ch, void *me, int cmd, char *argument);
int horse_keeper(CharData *ch, void *me, int cmd, char *argument);

namespace Mlist {

std::string print_race(CharData *mob) {
	std::string out;
	if (GET_RACE(mob) <= ENpcRace::kLastNpcRace) {
		out += npc_race_types[GET_RACE(mob) - ENpcRace::kBasic];
	} else {
		out += "UNDEF";
	}
	return out;
}

std::string print_role(CharData *mob) {
	std::string out;
	if (mob->get_role_bits().any()) {
		print_bitset(mob->get_role_bits(), npc_role_types, ",", out);
	} else {
		out += "---";
	}
	return out;
}

std::string print_script(CharData *mob, const std::string &key) {
	std::string out;

	bool print_name = false;
	if (key == "scriptname" || key == "triggername") {
		print_name = true;
	}

	if (!mob_proto[GET_MOB_RNUM(mob)].proto_script->empty()) {
		bool first = true;
		for (const auto trigger_vnum : *mob_proto[GET_MOB_RNUM(mob)].proto_script) {
			const int trg_rnum = real_trigger(trigger_vnum);
			if (trg_rnum >= 0) {
				if (!first) {
					out += ", ";
				} else {
					first = false;
				}
				out += boost::lexical_cast<std::string>(trig_index[trg_rnum]->vnum);
				if (print_name) {
					out += "(";
					const auto &trigger_name = trig_index[trg_rnum]->proto->get_name();
					out += !trigger_name.empty() ? trigger_name.c_str() : "null";
					out += ")";
				}
			}
		}
	} else {
		out += "---";
	}

	return out;
}

std::string print_special(CharData *mob) {
	std::string out;

	if (mob_index[GET_MOB_RNUM(mob)].func) {
		auto func = mob_index[GET_MOB_RNUM(mob)].func;
		if (func == shop_ext)
			out += "shop";
		else if (func == receptionist)
			out += "rent";
		else if (func == postmaster)
			out += "mail";
		else if (func == bank)
			out += "bank";
		else if (func == exchange)
			out += "exchange";
		else if (func == horse_keeper)
			out += "horse";
		else if (func == DoGuildLearn)
			out += "guild trainer";
		else if (func == torc)
			out += "torc";
		else if (func == Noob::outfit)
			out += "outfit";
	} else {
		out += "---";
	}

	return out;
}

std::string print_flag(CharData *ch, CharData *mob, const std::string &options) {
	std::vector<std::string> option_list;
	boost::split(option_list, options, boost::is_any_of(", "), boost::token_compress_on);

	std::string out;
	for (const auto & i : option_list) {
		if (isname(i, "race")) {
			out += boost::str(boost::format(" [раса: %s%s%s ]")
								  % CCCYN(ch, C_NRM) % print_race(mob) % CCNRM(ch, C_NRM));
		} else if (isname(i, "role")) {
			out += boost::str(boost::format(" [роли: %s%s%s ]")
								  % CCCYN(ch, C_NRM) % print_role(mob) % CCNRM(ch, C_NRM));
		} else if (isname(i, "script trigger scriptname triggername")) {
			out += boost::str(boost::format(" [скрипты: %s%s%s ]")
								  % CCCYN(ch, C_NRM) % print_script(mob, i) % CCNRM(ch, C_NRM));
		} else if (isname(i, "special")) {
			out += boost::str(boost::format(" [спец-проц: %s%s%s ]")
								  % CCCYN(ch, C_NRM) % print_special(mob) % CCNRM(ch, C_NRM));
		}
	}

	return out;
}

void print(CharData *ch, int first, int last, const std::string &options) {
	std::stringstream out;
	out << "Список мобов от " << first << " до " << last << "\r\n";
	int cnt = 0;
	for (int i = 0; i <= top_of_mobt; ++i) {
		if (mob_index[i].vnum >= first && mob_index[i].vnum <= last) {
			out << boost::format("%5d. %45s [%6d] [%2d]%s")
				% ++cnt
				% (mob_proto[i].get_name_str().size() > 45
				   ? mob_proto[i].get_name_str().substr(0, 45)
				   : mob_proto[i].get_name_str())
				% mob_index[i].vnum
				% mob_proto[i].GetLevel()
				% print_flag(ch, mob_proto + i, options);
			if (!mob_proto[i].proto_script->empty()) {
				out << " - есть скрипты -";
				for (const auto trigger_vnum : *mob_proto[i].proto_script) {
					sprintf(buf1, " [%d]", trigger_vnum);
					out << buf1;
				}
			} else
				out << " - нет скриптов";
			sprintf(buf1, " Всего в мире: %d\r\n", mob_index[i].total_online);
			out << buf1;
		}
	}

	if (cnt == 0) {
		SendMsgToChar("Нет мобов в этом промежутке.\r\n", ch);
	} else {
		page_string(ch->desc, out.str());
	}
}

} // namespace Mlist

int print_olist(const CharData *ch, const int first, const int last, std::string &out) {
	int result = 0;

	char buf_[256] = {0};
	std::stringstream ss;
	snprintf(buf_, sizeof(buf_), "Список объектов Vnum %d до %d\r\n", first, last);
	ss << buf_;

	auto from = obj_proto.vnum2index().lower_bound(first);
	auto to = obj_proto.vnum2index().upper_bound(last);
	for (auto i = from; i != to; ++i) {
		const auto vnum = i->first;
		const auto rnum = i->second;
		const auto prototype = obj_proto[rnum];
		snprintf(buf_, sizeof(buf_), "%5d. %s [%5d] [ilvl=%f : mort =%d]", ++result,
				 colored_name(prototype->get_short_description().c_str(), 45),
				 vnum, prototype->get_ilevel(), prototype->get_auto_mort_req());
		ss << buf_;

		if (GetRealLevel(ch) >= kLvlGreatGod
			|| PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
			snprintf(buf_, sizeof(buf_), " Игра:%d Пост:%d Макс:%d",
					 obj_proto.CountInWorld(rnum),
					 obj_proto.stored(rnum), GET_OBJ_MIW(obj_proto[rnum]));
			ss << buf_;

			const auto &script = prototype->get_proto_script();
			if (!script.empty()) {
				ss << " - есть скрипты -";
				for (const auto trigger_vnum : script) {
					sprintf(buf1, " [%d]", trigger_vnum);
					ss << buf1;
				}
			} else {
				ss << " - нет скриптов";
			}
		}

		ss << "\r\n";
	}

	out = ss.str();
	return result;
}

void do_liblist(CharData *ch, char *argument, int cmd, int subcmd) {

	int first, last, nr, found = 0;

	argument = two_arguments(argument, buf, buf2);
	first = atoi(buf);
	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) != first)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*buf || (!*buf2 && (subcmd == SCMD_ZLIST))) {
		switch (subcmd) {
			case SCMD_RLIST:
				SendMsgToChar("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			case SCMD_OLIST:
				SendMsgToChar("Использование: осписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			case SCMD_MLIST:
				SendMsgToChar("Использование: мсписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			case SCMD_ZLIST: SendMsgToChar("Использование: зсписок <начальный номер> <конечный номер>\r\n", ch);
				break;
			case SCMD_CLIST:
				SendMsgToChar("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			default: sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
				mudlog(buf, BRF, kLvlGod, SYSLOG, true);
				break;
		}
		return;
	}

	if (*buf2 && a_isdigit(buf2[0])) {
		last = atoi(buf2);
	} else {
		first *= 100;
		last = first + 99;
	}

	if ((first < 0) || (first > kMaxProtoNumber) || (last < 0) || (last > kMaxProtoNumber)) {
		sprintf(buf, "Значения должны быть между 0 и %d.\n\r", kMaxProtoNumber);
		SendMsgToChar(buf, ch);
		return;
	}

	if (first > last) {
		std::swap(first, last);
	}

	if (first + 100 < last) {
		SendMsgToChar("Максимальный показываемый промежуток - 100.\n\r", ch);
		return;
	}

	char buf_[256];
	std::string out;

	switch (subcmd) {
		case SCMD_RLIST:
			snprintf(buf_, sizeof(buf_),
					 "Список комнат от Vnum %d до %d\r\n", first, last);
			out += buf_;
			for (nr = kFirstRoom; nr <= top_of_world && (world[nr]->room_vn <= last); nr++) {
				if (world[nr]->room_vn >= first) {
					snprintf(buf_, sizeof(buf_), "%5d. [%5d] (%3d) %s",
							 ++found, world[nr]->room_vn, world[nr]->zone_rn, world[nr]->name);
					out += buf_;
					if (!world[nr]->proto_script->empty()) {
						out += " - есть скрипты -";
						for (const auto trigger_vnum : *world[nr]->proto_script) {
							sprintf(buf1, " [%d]", trigger_vnum);
							out += buf1;
						}
						out += "\r\n";
					} else {
						out += " - нет скриптов\r\n";
					}
				}
			}
			break;
		case SCMD_OLIST: found = print_olist(ch, first, last, out);
			break;

		case SCMD_MLIST: {
			std::string option;
			if (*buf2 && !a_isdigit(buf2[0])) {
				option = buf2;
			}
			option += argument;
			Mlist::print(ch, first, last, option);
			return;
		}
		case SCMD_ZLIST:
			snprintf(buf_, sizeof(buf_),
					 "Список зон от %d до %d\r\n"
					 "(флаги, номер, резет, уровень/средний уровень мобов, группа, имя)\r\n",
					 first, last);
			out += buf_;

			for (nr = 0; nr < static_cast<ZoneRnum>(zone_table.size()) && (zone_table[nr].vnum <= last); nr++) {
				if (zone_table[nr].vnum >= first) {
					snprintf(buf_, sizeof(buf_),
							 "%5d. [%s%s] [%5d] (%3d) (%2d/%2d) (%2d) %s\r\n",
							 ++found,
							 zone_table[nr].locked ? "L" : " ",
							 zone_table[nr].under_construction ? "T" : " ",
							 zone_table[nr].vnum,
							 zone_table[nr].lifespan,
							 zone_table[nr].level,
							 zone_table[nr].mob_level,
							 zone_table[nr].group,
							 zone_table[nr].name);
					out += buf_;
				}
			}
			break;

		case SCMD_CLIST: out = "Заглушка. Возможно, будет использоваться в будущем\r\n";
			break;

		default: sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
			mudlog(buf, BRF, kLvlGod, SYSLOG, true);
			return;
	}

	if (!found) {
		switch (subcmd) {
			case SCMD_RLIST: SendMsgToChar("Нет комнат в этом промежутке.\r\n", ch);
				break;
			case SCMD_OLIST: SendMsgToChar("Нет объектов в этом промежутке.\r\n", ch);
				break;
			case SCMD_ZLIST: SendMsgToChar("Нет зон в этом промежутке.\r\n", ch);
				break;
			default: sprintf(buf, "SYSERR:: invalid SCMD passed to do_build_list!");
				mudlog(buf, BRF, kLvlGod, SYSLOG, true);
				break;
		}
		return;
	}

	page_string(ch->desc, out);
}

void do_forcetime(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int m, t = 0;
	char *ca;

	// Parse command line
	for (ca = strtok(argument, " "); ca; ca = strtok(nullptr, " ")) {
		m = LOWER(ca[strlen(ca) - 1]);
		if (m == 'h')    // hours
			m = 60 * 60;
		else if (m == 'm')    // minutes
			m = 60;
		else if (m == 's')    // seconds
			m = 1;
		else
			m = 0;

		if ((m *= atoi(ca)) > 0)
			t += m;
		else {
			// no time shift with undefined arguments
			t = 0;
			break;
		}
	}

	if (t <= 0) {
		SendMsgToChar("Сдвиг игрового времени (h - часы, m - минуты, s - секунды).\r\n", ch);
			return;
	}

	for (m = 0; m < t * kPassesPerSec; m++) {
		GlobalObjects::heartbeat()(t * kPassesPerSec - m);
	}

	SendMsgToChar(ch, "Вы перевели игровое время на %d сек вперед.\r\n", t);
	sprintf(buf, "(GC) %s перевел игровое время на %d сек вперед.", GET_NAME(ch), t);
	mudlog(buf, NRM, kLvlImmortal, IMLOG, false);
	SendMsgToChar(OK, ch);

}

///////////////////////////////////////////////////////////////////////////////
//Polud статистика использования заклинаний
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

std::string statToPrint() {
	std::stringstream out;
	time_t now = time(nullptr);
	char *end_time = str_dup(rustime(localtime(&now)));
	out << rustime(localtime(&SpellUsage::start)) << " - " << end_time << "\n";
	for (auto & it : SpellUsage::usage) {
		out << std::setw(35) << MUD::Classes(it.first).GetName() << std::endl;
		for (auto & itt : it.second) {
			out << std::setw(25) << spell_info[itt.first].name << " : " << itt.second << std::endl;
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
	file << statToPrint();
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

void do_spellstat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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
		page_string(ch->desc, statToPrint());
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

void do_sanitize(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Запущена процедура сбора мусора после праздника...\r\n", ch);
	Celebrates::sanitize();
}

void do_loadstat(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
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

namespace {

struct filter_type {
	filter_type() : type(-1), wear(EWearFlag::kUndefined), wear_message(-1), material(-1) {};

	// тип
	int type;
	// куда одевается
	EWearFlag wear;
	// для названия куда одеть
	int wear_message;
	// материал
	int material;
	// аффекты weap
	std::vector<int> affect;
	// аффекты apply
	std::vector<int> affect2;
	// экстрафлаг
	std::vector<int> affect3;
};

} // namespace

void do_print_armor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || (!IS_GRGOD(ch) && !PRF_FLAGGED(ch, EPrf::kCoderinfo))) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	filter_type filter;
	char tmpbuf[kMaxInputLength];
	bool find_param = false;
	while (*argument) {
		switch (*argument) {
			case 'М': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbrev(tmpbuf, "булат")) {
					filter.material = EObjMaterial::kBulat;
				} else if (utils::IsAbbrev(tmpbuf, "бронза")) {
					filter.material = EObjMaterial::kBronze;
				} else if (utils::IsAbbrev(tmpbuf, "железо")) {
					filter.material = EObjMaterial::kIron;
				} else if (utils::IsAbbrev(tmpbuf, "сталь")) {
					filter.material = EObjMaterial::kSteel;
				} else if (utils::IsAbbrev(tmpbuf, "кованая.сталь")) {
					filter.material = EObjMaterial::kForgedSteel;
				} else if (utils::IsAbbrev(tmpbuf, "драг.металл")) {
					filter.material = EObjMaterial::kPreciousMetel;
				} else if (utils::IsAbbrev(tmpbuf, "кристалл")) {
					filter.material = EObjMaterial::kCrystal;
				} else if (utils::IsAbbrev(tmpbuf, "дерево")) {
					filter.material = EObjMaterial::kWood;
				} else if (utils::IsAbbrev(tmpbuf, "прочное.дерево")) {
					filter.material = EObjMaterial::kHardWood;
				} else if (utils::IsAbbrev(tmpbuf, "керамика")) {
					filter.material = EObjMaterial::kCeramic;
				} else if (utils::IsAbbrev(tmpbuf, "стекло")) {
					filter.material = EObjMaterial::kGlass;
				} else if (utils::IsAbbrev(tmpbuf, "камень")) {
					filter.material = EObjMaterial::kStone;
				} else if (utils::IsAbbrev(tmpbuf, "кость")) {
					filter.material = EObjMaterial::kBone;
				} else if (utils::IsAbbrev(tmpbuf, "ткань")) {
					filter.material = EObjMaterial::kCloth;
				} else if (utils::IsAbbrev(tmpbuf, "кожа")) {
					filter.material = EObjMaterial::kSkin;
				} else if (utils::IsAbbrev(tmpbuf, "органика")) {
					filter.material = EObjMaterial::kOrganic;
				} else if (utils::IsAbbrev(tmpbuf, "береста")) {
					filter.material = EObjMaterial::kPaper;
				} else if (utils::IsAbbrev(tmpbuf, "драг.камень")) {
					filter.material = EObjMaterial::kDiamond;
				} else {
					SendMsgToChar("Неверный материал предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'Т': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbrev(tmpbuf, "броня") || utils::IsAbbrev(tmpbuf, "armor")) {
					filter.type = EObjType::kArmor;
				} else if (utils::IsAbbrev(tmpbuf, "легкие") || utils::IsAbbrev(tmpbuf, "легкая")) {
					filter.type = EObjType::kLightArmor;
				} else if (utils::IsAbbrev(tmpbuf, "средние") || utils::IsAbbrev(tmpbuf, "средняя")) {
					filter.type = EObjType::kMediumArmor;
				} else if (utils::IsAbbrev(tmpbuf, "тяжелые") || utils::IsAbbrev(tmpbuf, "тяжелая")) {
					filter.type = EObjType::kHeavyArmor;
				} else {
					SendMsgToChar("Неверный тип предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'О': argument = one_argument(++argument, tmpbuf);
				if (utils::IsAbbrev(tmpbuf, "тело")) {
					filter.wear = EWearFlag::kBody;
					filter.wear_message = 3;
				} else if (utils::IsAbbrev(tmpbuf, "голова")) {
					filter.wear = EWearFlag::kHead;
					filter.wear_message = 4;
				} else if (utils::IsAbbrev(tmpbuf, "ноги")) {
					filter.wear = EWearFlag::kLegs;
					filter.wear_message = 5;
				} else if (utils::IsAbbrev(tmpbuf, "ступни")) {
					filter.wear = EWearFlag::kFeet;
					filter.wear_message = 6;
				} else if (utils::IsAbbrev(tmpbuf, "кисти")) {
					filter.wear = EWearFlag::kHands;
					filter.wear_message = 7;
				} else if (utils::IsAbbrev(tmpbuf, "руки")) {
					filter.wear = EWearFlag::kArms;
					filter.wear_message = 8;
				} else {
					SendMsgToChar("Неверное место одевания предмета.\r\n", ch);
					return;
				}
				find_param = true;
				break;
			case 'А': {
				bool tmp_find = false;
				argument = one_argument(++argument, tmpbuf);
				if (!strlen(tmpbuf)) {
					SendMsgToChar("Неверный аффект предмета.\r\n", ch);
					return;
				}
				if (filter.affect.size() + filter.affect2.size() + filter.affect3.size() >= 3) {
					break;
				}
				switch (*tmpbuf) {
					case '1': sprintf(tmpbuf, "можно вплавить 1 камень");
						break;
					case '2': sprintf(tmpbuf, "можно вплавить 2 камня");
						break;
					case '3': sprintf(tmpbuf, "можно вплавить 3 камня");
						break;
					default: break;
				}
				utils::ConvertToLow(tmpbuf);
				size_t len = strlen(tmpbuf);
				int num = 0;

				for (int flag = 0; flag < 4; ++flag) {
					for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num) {
						if (strlen(weapon_affects[num]) < len)
							continue;
						if (!strncmp(weapon_affects[num], tmpbuf, len)) {
							filter.affect.push_back(num);
							tmp_find = true;
							break;
						}
					}
					if (tmp_find) {
						break;
					}
					++num;
				}
				if (!tmp_find) {
					for (num = 0; *apply_types[num] != '\n'; ++num) {
						if (strlen(apply_types[num]) < len)
							continue;
						if (!strncmp(apply_types[num], tmpbuf, len)) {
							filter.affect2.push_back(num);
							tmp_find = true;
							break;
						}
					}
				}
				// поиск по экстрафлагу
				if (!tmp_find) {
					num = 0;
					for (int flag = 0; flag < 4; ++flag) {
						for (/* тут ничего не надо */; *extra_bits[num] != '\n'; ++num) {
							if (strlen(extra_bits[num]) < len)
								continue;
							if (!strncmp(extra_bits[num], tmpbuf, len)) {
								filter.affect3.push_back(num);
								tmp_find = true;
								break;
							}
						}
						if (tmp_find) {
							break;
						}
						num++;
					}
				}
				if (!tmp_find) {
					sprintf(buf, "Неверный аффект предмета: '%s'.\r\n", tmpbuf);
					SendMsgToChar(buf, ch);
					return;
				}
				find_param = true;
				break;
			}
			default: ++argument;
		}
	}
	if (!find_param) {
		SendMsgToChar("Формат команды:\r\n"
					  "   armor Т[броня|легкие|средние|тяжелые] О[тело|голова|ногиступни|кисти|руки] А[аффект] М[материал]\r\n",
					  ch);
		return;
	}
	std::string buffer = "Выборка по следующим параметрам: ";
	if (filter.material >= 0) {
		buffer += material_name[filter.material];
		buffer += " ";
	}
	if (filter.type >= 0) {
		buffer += item_types[filter.type];
		buffer += " ";
	}
	if (filter.wear != EWearFlag::kUndefined) {
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (!filter.affect.empty()) {
		for (const auto it : filter.affect) {
			buffer += weapon_affects[it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty()) {
		for (const auto it : filter.affect2) {
			buffer += apply_types[it];
			buffer += " ";
		}
	}
	if (!filter.affect3.empty()) {
		for (const auto it : filter.affect3) {
			buffer += extra_bits[it];
			buffer += " ";
		}
	}
	buffer += "\r\nСредний уровень мобов в зоне | внум предмета  | материал | имя предмета + аффекты если есть\r\n";
	SendMsgToChar(buffer, ch);

	std::multimap<int /* zone lvl */, int /* obj rnum */> tmp_list;
	for (const auto &i : obj_proto) {
		// материал
		if (filter.material >= 0 && filter.material != GET_OBJ_MATER(i)) {
			continue;
		}
		// тип
		if (filter.type >= 0 && filter.type != GET_OBJ_TYPE(i)) {
			continue;
		}
		// куда можно одеть
		if (filter.wear != EWearFlag::kUndefined
			&& !i->has_wear_flag(filter.wear)) {
			continue;
		}
		// аффекты
		bool find = true;
		if (!filter.affect.empty()) {
			for (int it : filter.affect) {
				if (!CompareBits(i->get_affect_flags(), weapon_affects, it)) {
					find = false;
					break;
				}
			}
			// аффект не найден, продолжать смысла нет
			if (!find) {
				continue;
			}
		}

		if (!filter.affect2.empty()) {
			for (auto it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it) {
				find = false;
				for (int k = 0; k < kMaxObjAffect; ++k) {
					if (i->get_affected(k).location == *it) {
						find = true;
						break;
					}
				}
			}
			// доп.свойство не найдено, продолжать смысла нет
			if (!find) {
				continue;
			}
		}
		if (!filter.affect3.empty()) {
			for (auto it = filter.affect3.begin(); it != filter.affect3.end() && find; ++it) {
				//find = true;
				if (!CompareBits(GET_OBJ_EXTRA(i), extra_bits, *it)) {
					find = false;
					break;
				}
			}
			// экстрафлаг не найден, продолжать смысла нет
			if (!find) {
				continue;
			}
		}

		if (find) {
			const auto vnum = i->get_vnum() / 100;
			for (auto & nr : zone_table) {
				if (vnum == nr.vnum) {
					tmp_list.insert(std::make_pair(nr.mob_level, GET_OBJ_RNUM(i)));
				}
			}
		}
	}

	std::ostringstream out;
	for (auto it = tmp_list.rbegin(), iend = tmp_list.rend(); it != iend; ++it) {
		const auto obj = obj_proto[it->second];
		out << "   "
			<< std::setw(2) << it->first << " | "
			<< std::setw(7) << obj->get_vnum() << " | "
			<< std::setw(14) << material_name[GET_OBJ_MATER(obj)] << " | "
			<< GET_OBJ_PNAME(obj, 0) << "\r\n";

		for (int i = 0; i < kMaxObjAffect; i++) {
			int drndice = obj->get_affected(i).location;
			int drsdice = obj->get_affected(i).modifier;
			if (drndice == EApply::kNone || !drsdice) {
				continue;
			}
			sprinttype(drndice, apply_types, buf2);
			bool negative = false;
			for (int j = 0; *apply_negative[j] != '\n'; j++) {
				if (!str_cmp(buf2, apply_negative[j])) {
					negative = true;
					break;
				}
			}
			if (obj->get_affected(i).modifier < 0) {
				negative = !negative;
			}
			snprintf(buf, kMaxStringLength, "   %s%s%s%s%s%d%s\r\n",
					 CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
					 CCCYN(ch, C_NRM),
					 negative ? " ухудшает на " : " улучшает на ", abs(drsdice), CCNRM(ch, C_NRM));
			out << "      |         |                | " << buf;
		}
	}
	if (!out.str().empty()) {
		SendMsgToChar(ch, "Всего найдено предметов: %lu\r\n\r\n", tmp_list.size());
		page_string(ch->desc, out.str());
	} else {
		SendMsgToChar("Ничего не найдено.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
