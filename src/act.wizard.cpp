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

#include "act.wizard.hpp"

#include "object.prototypes.hpp"
#include "world.objects.hpp"
#include "world.characters.hpp"
#include "logger.hpp"
#include "command.shutdown.hpp"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "skills.h"
#include "constants.h"
#include "olc.h"
#include "dg_scripts.h"
#include "pk.h"
#include "im.h"
#include "top.h"
#include "ban.hpp"
#include "description.h"
#include "title.hpp"
#include "names.hpp"
#include "password.hpp"
#include "privilege.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "genchar.h"
#include "file_crc.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "parcel.hpp"
#include "liquid.hpp"
#include "modify.h"
#include "room.hpp"
#include "glory_misc.hpp"
#include "glory_const.hpp"
#include "shop_ext.hpp"
#include "celebrates.hpp"
#include "player_races.hpp"
#include "birth_places.hpp"
#include "corpse.hpp"
#include "pugixml.hpp"
#include "sets_drop.hpp"
#include "fight.h"
#include "ext_money.hpp"
#include "noob.hpp"
#include "mail.h"
#include "mob_stat.hpp"
#include "char_obj_utils.inl"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "config.hpp"
#include "time_utils.hpp"
#include "global.objects.hpp"
#include "heartbeat.hpp"
#include "zone.table.hpp"

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

extern DESCRIPTOR_DATA *descriptor_list;
extern INDEX_DATA *mob_index;
extern char const *class_abbrevs[];
extern char const *kin_abbrevs[];
extern const char *weapon_affects[];
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern CHAR_DATA *mob_proto;
void medit_save_to_disk(int zone_num);
extern const char *Dirs[];
extern unsigned long int number_of_bytes_read;
extern unsigned long int number_of_bytes_written;
// for chars
extern const char *pc_class_types[];
extern struct spell_info_type spell_info[];
extern int check_dupes_host(DESCRIPTOR_DATA * d, bool autocheck = 0);
extern bool CompareBits(const FLAG_DATA& flags, const char *names[], int affect);	// to avoid inclusion of utils.h
void do_recall(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void save_zone_count_reset();
// extern functions
int level_exp(CHAR_DATA * ch, int level);
void appear(CHAR_DATA * ch);
void reset_zone(zone_rnum zone);
int parse_class(char arg);
extern CHAR_DATA *find_char(long n);
void rename_char(CHAR_DATA * ch, char *oname);
int _parse_name(char *arg, char *name);
int Valid_Name(char *name);
int reserved_word(const char *name);
int compute_armor_class(CHAR_DATA * ch);
extern bool can_be_reset(zone_rnum zone);
extern bool is_empty(zone_rnum zone_nr);
void list_feats(CHAR_DATA * ch, CHAR_DATA * vict, bool all_feats);
void list_skills(CHAR_DATA * ch, CHAR_DATA * vict, const char* filter = NULL);
void list_spells(CHAR_DATA * ch, CHAR_DATA * vict, int all_spells);
extern void print_rune_stats(CHAR_DATA *ch);
extern int real_zone(int number);
extern void reset_affects(CHAR_DATA *ch);
// local functions
int perform_set(CHAR_DATA * ch, CHAR_DATA * vict, int mode, char *val_arg);
void perform_immort_invis(CHAR_DATA * ch, int level);
void do_echo(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_send(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
room_rnum find_target_room(CHAR_DATA * ch, char *rawroomstr, int trig);
void do_at(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_goto(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_teleport(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_vnum(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stat_room(CHAR_DATA * ch, const int rnum = 0);
void do_stat_object(CHAR_DATA * ch, OBJ_DATA * j, const int virt = 0);//added by WorM virt при vstat'е 1 чтобы считалось реальное кол-во объектов в мире
void do_stat_character(CHAR_DATA * ch, CHAR_DATA * k, const int virt = 0);//added by WorM virt при vstat'е 1 чтобы считалось реальное кол-во мобов в мире
void do_statip(CHAR_DATA * ch, CHAR_DATA * k);
void do_stat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_shutdown(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void stop_snooping(CHAR_DATA * ch);
void do_snoop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_switch(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_return(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_load(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_vstat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_purge(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_inspect(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_syslog(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_advance(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void perform_immort_vis(CHAR_DATA * ch);
void do_invis(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gecho(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_poofset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_dc(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wizlock(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_date(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_last(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_force(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wiznet(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_zreset(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wizutil(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void print_zone_to_buf(char **bufptr, zone_rnum zone);
void do_show(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_set(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_liblist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
//
void do_godtest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sdemigod(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_unfreeze(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_setall(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_check_occupation(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_delete_obj(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_arena_restore(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_showzonestats(CHAR_DATA*, char*, int, int);
void do_overstuff(CHAR_DATA *ch, char*, int, int);
void do_send_text_to_char(CHAR_DATA *ch, char*, int, int);
void generate_magic_enchant(OBJ_DATA *obj);
void do_add_wizard(CHAR_DATA *ch, char*, int, int);

//extern std::vector<Stigma> stigmas;

void save_zone_count_reset()
{
	for (auto i = 0u; i < zone_table.size(); ++i)
	{
		sprintf(buf, "Zone: %d, count_reset: %d", zone_table[i].number, zone_table[i].count_reset);
		log("%s", buf);
	}
}

// Отправляет любой текст выбранному чару
void do_send_text_to_char(CHAR_DATA *ch, char *argument, int, int)
{
	CHAR_DATA *vict = NULL;

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
	{
		send_to_char("Кому и какой текст вы хотите отправить?\r\n", ch);
	}
	else if (!(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
	{
		send_to_char("Такого персонажа нет в игре.\r\n", ch);
	}
	else if (IS_NPC(vict))
		send_to_char("Такого персонажа нет в игре.\r\n", ch);
	else
	{
		snprintf(buf1, MAX_STRING_LENGTH, "%s\r\n", buf2);
		send_to_char(buf1, vict);
	}
}


// добавляет что-нибудь чару (пока что только стигмы)
void do_add_wizard(CHAR_DATA *, char *, int, int)
{
	/*CHAR_DATA *vict = NULL;

	half_chop(argument, buf, buf2);
	one_argument(buf2, buf1);

	if (!*buf || !*buf2)
	{
		send_to_char("Что и кому вы хотите добавить?\r\n", ch);
	}
	else if (!(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
	{
		send_to_char("Такого персонажа нет в игре.\r\n", ch);
	}
	else if (IS_NPC(vict))
		send_to_char("Такого персонажа нет в игре.\r\n", ch);
	else
	{
		int id_stigma = atoi(buf2);
		int stigma_wear = atoi(buf1);
		for (auto stigma : stigmas)
		{
			if (stigma.id == id_stigma)
			{
				ch->add_stigma(stigma_wear, id_stigma);
			}
		}
	}*/
}


// показывает количество вещей (чтобы носить которые, нужно больше 8 ремортов) в хранах кланов
void do_overstuff(CHAR_DATA *ch, char*, int, int)
{
	std::map<std::string, int> objects;
	for (Clan::ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		for (OBJ_DATA *chest = world[real_room((*clan)->get_chest_room())]->contents; chest; chest = chest->get_next_content())
		{
			if (Clan::is_clan_chest(chest))
			{
				for (OBJ_DATA *temp = chest->get_contains(); temp; temp = temp->get_next_content())
				{
					if (temp->get_auto_mort_req() > 8)
					{
						if (objects.count((*clan)->get_abbrev()))
						{
							objects[(*clan)->get_abbrev()] += 1;
						}
						else
						{
							objects.insert(std::pair<std::string, int>((*clan)->get_abbrev(), 1));
						}
					}
				}
			}
		}
	}

	for (auto it = objects.begin(); it != objects.end(); ++it)///вывод на экран
	{
		sprintf(buf, "Дружина: %s, количество объектов: %d\r\n", it->first.c_str(), it->second);
		send_to_char(buf, ch);
	}
}


// Функция для отправки текста богам
// При demigod = True, текст отправляется и демигодам тоже
void send_to_gods(char *text, bool demigod)
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		// Чар должен быть в игре
		if (STATE(d) == CON_PLAYING)
		{
			// Чар должен быть имморталом
			// Либо же демигодом (при demigod = true)
			if ((GET_LEVEL(d->character) >= LVL_GOD) ||
				(GET_GOD_FLAG(d->character, GF_DEMIGOD) && demigod))
			{
				send_to_char(text, d->character.get());
			}
		}
	}
}

#define MAX_TIME 0x7fffffff

extern const char *deaf_social;


// Adds karma string to KARMA
void add_karma(CHAR_DATA * ch, const char * punish, const char * reason)
{
	if (reason && (reason[0] != '.'))
	{
		time_t nt = time(NULL);
		sprintf(buf1, "%s :: %s [%s]\r\n", rustime(localtime(&nt)), punish, reason);
		KARMA(ch) = str_add(KARMA(ch), buf1);
	};
}

void do_delete_obj(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int vnum;
	one_argument(argument, buf);
	int num = 0;
	if (!*buf || !a_isdigit(*buf))
	{
		send_to_char("Usage: delete <number>\r\n", ch);
		return;
	}
	if ((vnum = atoi(buf)) < 0)
	{
		send_to_char("Указан неверный VNUM объекта !\r\n", ch);
		return;
	}

	world_objects.foreach_with_vnum(vnum, [&](const OBJ_DATA::shared_ptr& k)
	{
		k->set_timer(0);
		++num;
	});

	num += Depot::delete_obj(vnum);
	num += Clan::delete_obj(vnum);
	num += Parcel::delete_obj(vnum);

	sprintf(buf2, "Удалено всего предметов: %d", num);
	send_to_char(buf2, ch);

}

void do_showzonestats(CHAR_DATA* ch, char*, int, int)
{
	std::string buffer = "";
	for (auto i = 0u; i < zone_table.size(); ++i)
	{
		sprintf(buf, "Zone: %d, count_reset: %d, посещено с ребута: %d", zone_table[i].number, zone_table[i].count_reset, zone_table[i].traffic);
		buffer += std::string(buf) + "\r\n";
	}
	page_string(ch->desc, buffer);
}

void do_arena_restore(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else
	{
		GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
		GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
		if (IS_MANA_CASTER(vict))
		{
			GET_MANA_STORED(vict) = GET_MAX_MANA(vict);
		}
		else
		{
			GET_MEM_COMPLETED(vict) = GET_MEM_TOTAL(vict);
		}
		if (GET_CLASS(vict) == CLASS_WARRIOR)
		{
			struct timed_type wctimed;
			wctimed.skill = SKILL_WARCRY;
			wctimed.time = 0;
			timed_to_char(vict, &wctimed);
		}
		if (IS_GRGOD(ch) && IS_IMMORTAL(vict))
		{
			vict->set_str(25);
			vict->set_int(25);
			vict->set_wis(25);
			vict->set_dex(25);
			vict->set_con(25);
			vict->set_cha(25);
		}
		update_pos(vict);
		affect_from_char(vict, SPELL_DRUNKED);
		GET_DRUNK_STATE(vict) = GET_COND(vict, DRUNK) = 0;
		affect_from_char(vict, SPELL_ABSTINENT);

		//сброс таймеров скиллов и фитов
		while (vict->timed)
			timed_from_char(vict, vict->timed);
		while (vict->timed_feat)
			timed_feat_from_char(vict, vict->timed_feat);
		reset_affects(vict);
		for (int i = 0; i < NUM_WEARS; i++)
		{
			if (GET_EQ(vict, i))
			{
				remove_otrigger(GET_EQ(vict, i), vict);
				extract_obj(unequip_char(vict, i));
			}
		}
		OBJ_DATA *obj;
		for (obj = vict->carrying; obj; obj = vict->carrying)
		{
			obj_from_char(obj);
			extract_obj(obj);
		}
		act("Все ваши вещи были удалены и все аффекты сняты $N4!", FALSE, vict, 0, ch, TO_CHAR);
	}
}

int set_punish(CHAR_DATA * ch, CHAR_DATA * vict, int punish, char * reason, long times)
{
	punish_data * pundata = 0;
	int result;
	if (ch == vict)
	{
		send_to_char("Это слишком жестоко...\r\n", ch);
		return 0;
	}

	if ((GET_LEVEL(vict) >= LVL_IMMORT && !IS_IMPL(ch)) || IS_IMPL(vict))
	{
		send_to_char("Кем вы себя возомнили?\r\n", ch);
		return 0;
	}

	// Проверяем а может ли чар вообще работать с этим наказанием.
	switch (punish)
	{
	case SCMD_MUTE:
		pundata = &(vict)->player_specials->pmute;
		break;
	case SCMD_DUMB:
		pundata = &(vict)->player_specials->pdumb;
		break;
	case SCMD_HELL:
		pundata = &(vict)->player_specials->phell;
		break;
	case SCMD_NAME:
		pundata = &(vict)->player_specials->pname;
		break;

	case SCMD_FREEZE:
		pundata = &(vict)->player_specials->pfreeze;
		break;

	case SCMD_REGISTER:
	case SCMD_UNREGISTER:
		pundata = &(vict)->player_specials->punreg;
		break;
	}
	assert(pundata);
	if (GET_LEVEL(ch) < pundata->level && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char("Да кто ты такой!!? Чтобы оспаривать волю СТАРШИХ БОГОВ !!!\r\n", ch);
		return 0;
	}

	// Проверяем наказание или освобождение.
	if (times == 0)
	{
		// Чара досрочно освобождают от наказания.
		if (!reason || !*reason)
		{
			send_to_char("Укажите причину такой милости.\r\n", ch);
			return 0;
		}
		else
			skip_spaces(&reason);
		//

		pundata->duration = 0;
		pundata->level = 0;
		pundata->godid = 0;

		if (pundata->reason)
			free(pundata->reason);

		pundata->reason = 0;

		switch (punish)
		{
		case SCMD_MUTE:
			if (!PLR_FLAGGED(vict, PLR_MUTE))
			{
				send_to_char("Ваша жертва и так может кричать.\r\n", ch);
				return (0);
			};
			PLR_FLAGS(vict).unset(PLR_MUTE);

			sprintf(buf, "Mute OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Mute OFF by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s разрешил$G вам кричать.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n2 вернулся голос.");
			break;
		case SCMD_FREEZE:
			if (!PLR_FLAGGED(vict, PLR_FROZEN))
			{
				send_to_char("Ваша жертва уже разморожена.\r\n", ch);
				return (0);
			};
			PLR_FLAGS(vict).unset(PLR_FROZEN);
			Glory::remove_freeze(GET_UNIQUE(vict));

			sprintf(buf, "Freeze OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Freeze OFF by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);
			if (IN_ROOM(vict) != NOWHERE)
			{
				act("$n выпущен$a из темницы!", FALSE, vict, 0, 0, TO_ROOM);

				if ((result = GET_LOADROOM(vict)) == NOWHERE)
					result = calc_loadroom(vict);

				result = real_room(result);

				if (result == NOWHERE)
				{
					if (GET_LEVEL(vict) >= LVL_IMMORT)
						result = r_immort_start_room;
					else
						result = r_mortal_start_room;
				}
				char_from_room(vict);
				char_to_room(vict, result);
				look_at_room(vict, result);
			};

			sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n выпущен$a из темницы!");

			sprintf(buf, "%sЛедяные оковы растаяли под добрым взглядом $N1.%s",
				CCIYEL(vict, C_NRM), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n2 освободился из ледяного плена.");
			break;

		case SCMD_DUMB:
			if (!PLR_FLAGGED(vict, PLR_DUMB))
			{
				send_to_char("Ваша жертва и так может издавать звуки.\r\n", ch);
				return (0);
			};
			PLR_FLAGS(vict).unset(PLR_DUMB);

			sprintf(buf, "Dumb OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Dumb OFF by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s разрешил$G вам издавать звуки.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n нарушил$g обет молчания.");

			break;

		case SCMD_HELL:
			if (!PLR_FLAGGED(vict, PLR_HELLED))
			{
				send_to_char("Ваша жертва и так на свободе.\r\n", ch);
				return (0);
			};
			PLR_FLAGS(vict).unset(PLR_HELLED);

			sprintf(buf, "%s removed FROM hell by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Removed FROM hell by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

			if (IN_ROOM(vict) != NOWHERE)
			{
				act("$n выпущен$a из темницы!", FALSE, vict, 0, 0, TO_ROOM);

				if ((result = GET_LOADROOM(vict)) == NOWHERE)
					result = calc_loadroom(vict);

				result = real_room(result);

				if (result == NOWHERE)
				{
					if (GET_LEVEL(vict) >= LVL_IMMORT)
						result = r_immort_start_room;
					else
						result = r_mortal_start_room;
				}
				char_from_room(vict);
				char_to_room(vict, result);
				look_at_room(vict, result);
			};

			sprintf(buf, "%s%s выпустил$G вас из темницы.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n выпущен$a из темницы!");
			break;

		case SCMD_NAME:

			if (!PLR_FLAGGED(vict, PLR_NAMED))
			{
				send_to_char("Вашей жертвы там нет.\r\n", ch);
				return (0);
			};
			PLR_FLAGS(vict).unset(PLR_NAMED);

			sprintf(buf, "%s removed FROM name room by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Removed FROM name room by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

			if (IN_ROOM(vict) != NOWHERE)
			{
				if ((result = GET_LOADROOM(vict)) == NOWHERE)
					result = calc_loadroom(vict);

				result = real_room(result);

				if (result == NOWHERE)
				{
					if (GET_LEVEL(vict) >= LVL_IMMORT)
						result = r_immort_start_room;
					else
						result = r_mortal_start_room;
				}

				char_from_room(vict);
				char_to_room(vict, result);
				look_at_room(vict, result);
				act("$n выпущен$a из комнаты имени!", FALSE, vict, 0, 0, TO_ROOM);
			};
			sprintf(buf, "%s%s выпустил$G вас из комнаты имени.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n выпущен$a из комнаты имени!");
			break;

		case SCMD_REGISTER:
			// Регистриуем чара
			if (PLR_FLAGGED(vict, PLR_REGISTERED))
			{
				send_to_char("Вашей жертва уже зарегистрирована.\r\n", ch);
				return (0);
			};

			sprintf(buf, "%s registered by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Registered by %s", GET_NAME(ch));
			RegisterSystem::add(vict, buf, reason);
			add_karma(vict, buf, reason);

			if (IN_ROOM(vict) != NOWHERE)
			{

				act("$n зарегистрирован$a!", FALSE, vict, 0, 0, TO_ROOM);

				if ((result = GET_LOADROOM(vict)) == NOWHERE)
					result = calc_loadroom(vict);

				result = real_room(result);

				if (result == NOWHERE)
				{
					if (GET_LEVEL(vict) >= LVL_IMMORT)
						result = r_immort_start_room;
					else
						result = r_mortal_start_room;
				}

				char_from_room(vict);
				char_to_room(vict, result);
				look_at_room(vict, result);
			};
			sprintf(buf, "%s%s зарегистрировал$G вас.%s",
				CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!");
			break;
		case SCMD_UNREGISTER:
			if (!PLR_FLAGGED(vict, PLR_REGISTERED))
			{
				send_to_char("Ваша цель и так не зарегистрирована.\r\n", ch);
				return (0);
			};

			sprintf(buf, "%s unregistered by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Unregistered by %s", GET_NAME(ch));
			RegisterSystem::remove(vict);
			add_karma(vict, buf, reason);

			if (IN_ROOM(vict) != NOWHERE)
			{
				act("C $n1 снята метка регистрации!", FALSE, vict, 0, 0, TO_ROOM);
				/*				if ((result = GET_LOADROOM(vict)) == NOWHERE)
									result = r_unreg_start_room;

								result = real_room(result);

								if (result == NOWHERE)
								{
									if (GET_LEVEL(vict) >= LVL_IMMORT)
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
	}
	else
	{
		// Чара наказывают.
		if (!reason || !*reason)
		{
			send_to_char("Укажите причину наказания.\r\n", ch);
			return 0;
		}
		else
			skip_spaces(&reason);

		pundata->level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
		pundata->godid = GET_UNIQUE(ch);

		// Добавляем в причину имя имма

		sprintf(buf, "%s : %s", GET_NAME(ch), reason);
		pundata->reason = str_dup(buf);

		switch (punish)
		{
		case SCMD_MUTE:
			PLR_FLAGS(vict).set(PLR_MUTE);
			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			sprintf(buf, "Mute ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Mute ON (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s запретил$G вам кричать.%s",
				CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n подавился своим криком.");

			break;

		case SCMD_FREEZE:
			PLR_FLAGS(vict).set(PLR_FROZEN);
			Glory::set_freeze(GET_UNIQUE(vict));
			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			sprintf(buf, "Freeze ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Freeze ON (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%sАдский холод сковал ваше тело ледяным панцирем.\r\n%s",
				CCIBLU(vict, C_NRM), CCNRM(vict, C_NRM));
			sprintf(buf2, "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.");
			if (IN_ROOM(vict) != NOWHERE)
			{
				act("$n водворен$a в темницу!", FALSE, vict, 0, 0, TO_ROOM);

				char_from_room(vict);
				char_to_room(vict, r_helled_start_room);
				look_at_room(vict, r_helled_start_room);
			};
			break;


		case SCMD_DUMB:
			PLR_FLAGS(vict).set(PLR_DUMB);
			pundata->duration = (times > 0) ? time(NULL) + times * 60 : MAX_TIME;

			sprintf(buf, "Dumb ON for %s by %s(%ldm).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);

			sprintf(buf, "Dumb ON (%ldm) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s запретил$G вам издавать звуки.%s",
				CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n дал$g обет молчания.");
			break;
		case SCMD_HELL:
			PLR_FLAGS(vict).set(PLR_HELLED);

			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			if (IN_ROOM(vict) != NOWHERE)
			{
				act("$n водворен$a в темницу!", FALSE, vict, 0, 0, TO_ROOM);

				char_from_room(vict);
				char_to_room(vict, r_helled_start_room);
				look_at_room(vict, r_helled_start_room);
			};
			vict->set_was_in_room(NOWHERE);

			sprintf(buf, "%s moved TO hell by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Moved TO hell (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s поместил$G вас в темницу.%s", GET_NAME(ch),
				CCIRED(vict, C_NRM), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n водворен$a в темницу!");
			break;

		case SCMD_NAME:
			PLR_FLAGS(vict).set(PLR_NAMED);

			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			if (IN_ROOM(vict) != NOWHERE)
			{
				act("$n водворен$a в комнату имени!", FALSE, vict, 0, 0, TO_ROOM);
				char_from_room(vict);
				char_to_room(vict, r_named_start_room);
				look_at_room(vict, r_named_start_room);
			};
			vict->set_was_in_room(NOWHERE);

			sprintf(buf, "%s removed to nameroom by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Removed TO nameroom (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s поместил$G вас в комнату имени.%s",
				CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n помещен$a в комнату имени!");
			break;

		case SCMD_UNREGISTER:
			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
			RegisterSystem::remove(vict);

			if (IN_ROOM(vict) != NOWHERE)
			{
				if (vict->desc && !check_dupes_host(vict->desc) && IN_ROOM(vict) != r_unreg_start_room)
				{
					act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.", FALSE, vict, 0, 0, TO_ROOM);
					char_from_room(vict);
					char_to_room(vict, r_unreg_start_room);
					look_at_room(vict, r_unreg_start_room);
				}
			}
			vict->set_was_in_room(NOWHERE);

			sprintf(buf, "%s unregistred by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s", buf);
			sprintf(buf, "Unregistered (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s снял$G с вас... регистрацию :).%s",
				CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n лишен$a регистрации!");

			break;

		}
	}
	if (ch->in_room != NOWHERE)
	{
		act(buf, FALSE, vict, 0, ch, TO_CHAR);
		act(buf2, FALSE, vict, 0, ch, TO_ROOM);
	};
	return 1;
}



void is_empty_ch(zone_rnum zone_nr, CHAR_DATA *ch)
{
	DESCRIPTOR_DATA *i;
	int rnum_start, rnum_stop;
	bool found = false;
	CHAR_DATA *caster;
	//Проверим, нет ли в зоне метки для врат, чтоб не абузили.
	for (auto it = RoomSpells::aff_room_list.begin(); it != RoomSpells::aff_room_list.end(); ++it)
	{
		const auto aff = find_room_affect(*it, SPELL_RUNE_LABEL);
		if (((*it)->zone == zone_nr)
			&& aff != (*it)->affected.end())
		{
			// если в зоне метка
			caster = find_char((*aff)->caster_id);
			if (caster)
			{
				sprintf(buf2, "В зоне vnum:%d клетка vnum: %d находится рунная метка игрока: %s.\r\n", zone_table[zone_nr].number, (*it)->number, GET_NAME(caster));
				send_to_char(buf2, ch);
			}
		}
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == NOWHERE)
			continue;
		if (GET_LEVEL(i->character) >= LVL_IMMORT)
			continue;
		if (world[i->character->in_room]->zone != zone_nr)
			continue;
		sprintf(buf2, "Проверка по дискрипторам: В зоне (vnum: %d клетка: %d) находится персонаж: %s.\r\n", zone_table[zone_nr].number, GET_ROOM_VNUM(IN_ROOM(i->character)), GET_NAME(i->character));
		send_to_char(buf2, ch);
		found = true;
	}
	if (found)
		return;
	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!get_zone_rooms(zone_nr, &rnum_start, &rnum_stop))
	{
		sprintf(buf2, "Нет комнат в зоне %d.", static_cast<int>(zone_table[zone_nr].number));
		send_to_char(buf2, ch);
		return;	// в зоне нет комнат :)
	}

	for (; rnum_start <= rnum_stop; rnum_start++)
	{
		// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		{
			for (const auto c : world[rnum_start]->people)
			{
				if (!IS_NPC(c) && (GET_LEVEL(c) < LVL_IMMORT))
				{
					sprintf(buf2, "Проверка по списку чаров (с учетом linkdrop): в зоне vnum: %d клетка: %d находится персонаж: %s.\r\n", zone_table[zone_nr].number, GET_ROOM_VNUM(IN_ROOM(c)), GET_NAME(c));
					send_to_char(buf2, ch);
					found = true;
				}
			}
		}
	}

	// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (const auto c : world[STRANGE_ROOM]->people)
	{
		int was = c->get_was_in_room();
		if (was == NOWHERE
			|| GET_LEVEL(c) >= LVL_IMMORT
			|| world[was]->zone != zone_nr)
		{
			continue;
		}

		sprintf(buf2, "В прокси руме сидит игрок %s находящийся в зоне vnum: %d клетка: %d\r\n", GET_NAME(c), zone_table[zone_nr].number, GET_ROOM_VNUM(IN_ROOM(c)));
		send_to_char(buf2, ch);
		found = true;
	}

	if (!found)
	{
		sprintf(buf2, "В зоне %d даже мышь не пробегала.\r\n", zone_table[zone_nr].number);
		send_to_char(buf2, ch);
	}
}

void do_check_occupation(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int number;
	zone_rnum zrn;
	one_argument(argument, buf);
	bool is_found = false;
	if (!*buf || !a_isdigit(*buf))
	{
		send_to_char("Usage: занятость внумзоны\r\n", ch);
		return;
	}

	if ((number = atoi(buf)) < 0)
	{
		send_to_char("Такого внума не может быть!\r\n", ch);
		return;
	}

	// что-то по другому не нашел, как проверить существует такая зона или нет
	for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
	{
		if (zone_table[zrn].number == number)
		{
			is_empty_ch(zrn, ch);
			is_found = true;
			break;
		}
	}

	if (!is_found)
	{
		send_to_char("Такой зоны нет.\r\n", ch);
	}
}

SetAllInspReqListType& setall_inspect_list = GlobalObjects::setall_inspect_list();
InspReqListType& inspect_list = GlobalObjects::inspect_list();

#define SETALL_FREEZE 0
#define SETALL_EMAIL 1
#define SETALL_PSWD 2
#define SETALL_HELL 3

void setall_inspect()
{
	if (setall_inspect_list.size() == 0)
		return;
	SetAllInspReqListType::iterator it = setall_inspect_list.begin();
	CHAR_DATA *ch = 0;
	DESCRIPTOR_DATA *d_vict = 0;

	DESCRIPTOR_DATA *imm_d = DescByUID(player_table[it->first].unique);
	if (!imm_d
		|| (STATE(imm_d) != CON_PLAYING)
		|| !(ch = imm_d->character.get()))
	{
		setall_inspect_list.erase(it->first);
		return;
	}

	struct timeval start, stop, result;
	int is_online;
	need_warn = false;
	gettimeofday(&start, NULL);
	Player *vict;
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++)
	{
		vict = new Player;
		gettimeofday(&stop, NULL);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= OPT_USEC)
		{
			delete vict;
			return;
		}
		buf1[0] = '\0';
		is_online = 0;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict)
			is_online = 1;
		if (player_table[it->second->pos].mail)
			if (strstr(player_table[it->second->pos].mail, it->second->mail))
			{
				it->second->found++;
				if (it->second->type_req == SETALL_FREEZE)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						set_punish(imm_d->character.get(), d_vict->character.get(), SCMD_FREEZE, it->second->reason, it->second->freeze_time);
					}
					else
					{
						if (load_char(player_table[it->second->pos].name(), vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						}
						else
						{
							if (GET_LEVEL(vict) >= LVL_GOD)
							{
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							set_punish(imm_d->character.get(), vict, SCMD_FREEZE, it->second->reason, it->second->freeze_time);
							vict->save_char();
						}
					}
				}
				else if (it->second->type_req == SETALL_EMAIL)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						strncpy(GET_EMAIL(d_vict->character), it->second->newmail, 127);
						*(GET_EMAIL(d_vict->character) + 127) = '\0';
						sprintf(buf2, "Смена e-mail адреса персонажа %s с %s на %s.\r\n", player_table[it->second->pos].name(), player_table[it->second->pos].mail, it->second->newmail);
						add_karma(d_vict->character.get(), buf2, GET_NAME(imm_d->character));
						it->second->out += buf2;

					}
					else
					{
						if (load_char(player_table[it->second->pos].name(), vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						}
						else
						{
							if (GET_LEVEL(vict) >= LVL_GOD)
							{
								it->second->out += buf1;
								continue;
							}
							strncpy(GET_EMAIL(vict), it->second->newmail, 127);
							*(GET_EMAIL(vict) + 127) = '\0';
							sprintf(buf2, "Смена e-mail адреса персонажа %s с %s на %s.\r\n", player_table[it->second->pos].name(), player_table[it->second->pos].mail, it->second->newmail);
							it->second->out += buf2;
							add_karma(vict, buf2, GET_NAME(imm_d->character));
							vict->save_char();
						}
					}
				}
				else if (it->second->type_req == SETALL_PSWD)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
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
					}
					else
					{
						if (load_char(player_table[it->second->pos].name(), vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							delete vict;
							continue;
						}
						if (GET_LEVEL(vict) >= LVL_GOD)
						{
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
				}
				else if (it->second->type_req == SETALL_HELL)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
							it->second->out += buf1;
							continue;
						}
						set_punish(imm_d->character.get(), d_vict->character.get(), SCMD_HELL, it->second->reason, it->second->freeze_time);
					}
					else
					{
						if (load_char(player_table[it->second->pos].name(), vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки персонажа: %s.\r\n", player_table[it->second->pos].name());
							delete vict;
							it->second->out += buf1;
							continue;
						}
						else
						{
							if (GET_LEVEL(vict) >= LVL_GOD)
							{
								sprintf(buf1, "Персонаж %s бессмертный!\r\n", player_table[it->second->pos].name());
								it->second->out += buf1;
								continue;
							}
							set_punish(imm_d->character.get(), vict, SCMD_HELL, it->second->reason, it->second->freeze_time);
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
	gettimeofday(&stop, NULL);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d.\r\n", it->second->found);
	it->second->out += buf1;
	page_string(ch->desc, it->second->out);
	setall_inspect_list.erase(it->first);
}

void do_setall(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int type_request = 0;
	int times = 0;
	if (ch->get_pfilepos() < 0)
		return;

	InspReqListType::iterator it_inspect = inspect_list.find(GET_UNIQUE(ch));
	SetAllInspReqListType::iterator it = setall_inspect_list.find(GET_UNIQUE(ch));
	// На всякий случай разрешаем только одну команду такого типа - либо setall, либо inspect
	if (it_inspect != inspect_list.end() && it != setall_inspect_list.end())
	{
		send_to_char(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}

	argument = three_arguments(argument, buf, buf1, buf2);
	SetAllInspReqPtr req(new setall_inspect_request);
	req->newmail = NULL;
	req->mail = NULL;
	req->reason = NULL;
	req->pwd = NULL;

	if (!*buf)
	{
		send_to_char("Usage: setall <e-mail> <email|passwd|frozen|hell> <arguments>\r\n", ch);
		return;
	}

	if (!valid_email(buf))
	{
		send_to_char("Некорректный e-mail!\r\n", ch);
		return;
	}

	if (!isname(buf1, "frozen email passwd hell"))
	{
		send_to_char("Данное действие совершить нельзя.\r\n", ch);
		return;
	}
	if (is_abbrev(buf1, "frozen"))
	{
		skip_spaces(&argument);
		if (!argument || !*argument)
		{
			send_to_char("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = SETALL_FREEZE;
		req->freeze_time = times;
		req->reason = strdup(argument);
	}
	else if (is_abbrev(buf1, "email"))
	{
		if (!*buf2)
		{
			send_to_char("Укажите новый e-mail!\r\n", ch);
			return;
		}
		if (!valid_email(buf2))
		{
			send_to_char("Новый e-mail некорректен!\r\n", ch);
			return;
		}
		req->newmail = strdup(buf2);
		type_request = SETALL_EMAIL;
	}
	else if (is_abbrev(buf1, "passwd"))
	{
		if (!*buf2)
		{
			send_to_char("Укажите новый пароль!\r\n", ch);
			return;
		}
		req->pwd = strdup(buf2);
		type_request = SETALL_PSWD;
	}
	else if (is_abbrev(buf1, "hell"))
	{
		skip_spaces(&argument);
		if (!argument || !*argument)
		{
			send_to_char("Необходимо указать причину такой немилости.\r\n", ch);
			return;
		}
		if (*buf2) times = atol(buf2);
		type_request = SETALL_HELL;
		req->freeze_time = times;
		req->reason = strdup(argument);
	}
	else
	{
		send_to_char("Какой-то баг. Вы эту надпись видеть не должны.\r\n", ch);
		return;
	}

	req->type_req = type_request;
	req->mail = str_dup(buf);
	req->pos = 0;
	req->found = 0;
	req->out = "";
	setall_inspect_list[ch->get_pfilepos()] = req;
}

void do_echo(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	if (PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вы не в состоянии что-либо продемонстрировать окружающим.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("И что вы хотите выразить столь красочно?\r\n", ch);
	}
	else
	{
		if (subcmd == SCMD_EMOTE)
		{
			// added by Pereplut
			if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
			{
				if PLR_FLAGGED(ch->get_master(), PLR_DUMB)
				{
					// shapirus: правильно пишется не "так-же", а "так же".
					// и запятая пропущена была :-P.
					send_to_char("Ваши последователи так же немы, как и вы!\r\n", ch->get_master());
					return;
				}
			}
			sprintf(buf, "&K$n %s.&n", argument);
		}
		else
		{
			strcpy(buf, argument);
		}

		for (const auto to : world[ch->in_room]->people)
		{
			if (to == ch
				|| ignores(to, ch, IGNORE_EMOTE))
			{
				continue;
			}

			act(buf, FALSE, ch, 0, to, TO_VICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, to, TO_VICT | CHECK_NODEAF);
		}

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		{
			send_to_char(OK, ch);
		}
		else
		{
			act(buf, FALSE, ch, 0, 0, TO_CHAR);
		}
	}
}

#define SHOW_GLORY 	0
#define ADD_GLORY 	1
#define SUB_GLORY 	2
#define SUB_STATS 	3
#define SUB_TRANS 	4
#define SUB_HIDE    5

void do_glory(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	// Команда простановки славы (оффлайн/онлайн)
	// Без параметров выводит славу у игрока
	// + cлава прибавляет славу
	// - cлава убавляет славу
	char num[MAX_INPUT_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	int mode = 0;
	char *reason;

	if (!*argument)
	{
		send_to_char("Формат команды : \r\n"
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
	else if (is_abbrev(num, "remove"))
	{
		// тут у нас в num получается remove, в arg1 кол-во и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_STATS;
	}
	else if (is_abbrev(num, "transfer"))
	{
		// а тут в num transfer, в arg1 имя принимающего славу и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_TRANS;
	}
	else if (is_abbrev(num, "hide"))
	{
		// а тут в num hide, в arg1 on|off и в reason причина
		reason = any_one_arg(reason, arg1);
		skip_spaces(&reason);
		mode = SUB_HIDE;
	}

	// точки убираем, чтобы карма всегда писалась
	skip_dots(&reason);

	if (mode != SHOW_GLORY)
	{
		if ((reason == 0) || (*reason == 0))
		{
			send_to_char("Укажите причину изменения славы?\r\n", ch);
			return;
		}
	}

	CHAR_DATA *vict = get_player_vis(ch, arg, FIND_CHAR_WORLD);
	Player t_vict; // TODO: надо выносить во вторую функцию, чтобы зря не создавать
	if (!vict)
	{
		if (load_char(arg, &t_vict) < 0)
		{
			send_to_char("Такого персонажа не существует.\r\n", ch);
			return;
		}
		vict = &t_vict;
	}

	switch (mode)
	{
	case ADD_GLORY:
	{
		int amount = atoi((num + 1));
		Glory::add_glory(GET_UNIQUE(vict), amount);
		send_to_char(ch, "%s добавлено %d у.е. славы (Всего: %d у.е.).\r\n",
			GET_PAD(vict, 2), amount, Glory::get_glory(GET_UNIQUE(vict)));
		imm_log("(GC) %s sets +%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
		// запись в карму
		sprintf(buf, "Change glory +%d by %s", amount, GET_NAME(ch));
		add_karma(vict, buf, reason);
		GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
		break;
	}
	case SUB_GLORY:
	{
		int amount = Glory::remove_glory(GET_UNIQUE(vict), atoi((num + 1)));
		if (amount <= 0)
		{
			send_to_char(ch, "У %s нет свободной славы.", GET_PAD(vict, 1));
			break;
		}
		send_to_char(ch, "У %s вычтено %d у.е. славы (Всего: %d у.е.).\r\n",
			GET_PAD(vict, 1), amount, Glory::get_glory(GET_UNIQUE(vict)));
		imm_log("(GC) %s sets -%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
		// запись в карму
		sprintf(buf, "Change glory -%d by %s", amount, GET_NAME(ch));
		add_karma(vict, buf, reason);
		GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
		break;
	}
	case SUB_STATS:
	{
		if (Glory::remove_stats(vict, ch, atoi(arg1)))
		{
			sprintf(buf, "Remove stats %s by %s", arg1, GET_NAME(ch));
			add_karma(vict, buf, reason);
			GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
		}
		break;
	}
	case SUB_TRANS:
	{
		Glory::transfer_stats(vict, ch, arg1, reason);
		break;
	}
	case SUB_HIDE:
	{
		Glory::hide_char(vict, ch, arg1);
		sprintf(buf, "Hide %s by %s", arg1, GET_NAME(ch));
		add_karma(vict, buf, reason);
		GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
		break;
	}
	default:
		Glory::show_glory(vict, ch);
	}

	vict->save_char();
}

void do_send(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;

	half_chop(argument, arg, buf);

	if (!*arg)
	{
		send_to_char("Послать что и кому (не путать с куда и кого :)\r\n", ch);
		return;
	}
	if (!(vict = get_player_vis(ch, arg, FIND_CHAR_WORLD)))
	{
		send_to_char(NOPERSON, ch);
		return;
	}
	send_to_char(buf, vict);
	send_to_char("\r\n", vict);
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char("Послано.\r\n", ch);
	else
	{
		sprintf(buf2, "Вы послали '%s' %s.\r\n", buf, GET_PAD(vict, 2));
		send_to_char(buf2, ch);
	}
}

// take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93
room_rnum find_target_room(CHAR_DATA * ch, char *rawroomstr, int trig)
{
	room_vnum tmp;
	room_rnum location;
	CHAR_DATA *target_mob;
	OBJ_DATA *target_obj;
	char roomstr[MAX_INPUT_LENGTH];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr)
	{
		send_to_char("Укажите номер или название комнаты.\r\n", ch);
		return (NOWHERE);
	}
	if (a_isdigit(*roomstr) && !strchr(roomstr, '.'))
	{
		tmp = atoi(roomstr);
		if ((location = real_room(tmp)) == NOWHERE)
		{
			send_to_char("Нет комнаты с таким номером.\r\n", ch);
			return (NOWHERE);
		}
	}
	else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) != NULL)
	{
		location = target_mob->in_room;
	}
	else if ((target_obj = get_obj_vis(ch, roomstr)) != NULL)
	{
		if (target_obj->get_in_room() != NOWHERE)
		{
			location = target_obj->get_in_room();
		}
		else
		{
			send_to_char("Этот объект вам недоступен.\r\n", ch);
			return (NOWHERE);
		}
	}
	else
	{
		send_to_char("В округе нет похожего предмета или создания.\r\n", ch);
		return (NOWHERE);
	}

	// a location has been found -- if you're < GRGOD, check restrictions.
	if (!IS_GRGOD(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		if (ROOM_FLAGGED(location, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GRGOD)
		{
			send_to_char("Вы не столь божественны, чтобы получить доступ в эту комнату!\r\n", ch);
			return (NOWHERE);
		}
		if (ROOM_FLAGGED(location, ROOM_NOTELEPORTIN) && trig != 1)
		{
			send_to_char("В комнату не телепортировать!\r\n", ch);
			return (NOWHERE);
		}
		if (!Clan::MayEnter(ch, location, HCE_PORTAL))
		{
			send_to_char("Частная собственность - посторонним в ней делать нечего!\r\n", ch);
			return (NOWHERE);
		}
	}
	return (location);
}

void do_at(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	char command[MAX_INPUT_LENGTH];
	room_rnum location, original_loc;

	half_chop(argument, buf, command);
	if (!*buf)
	{
		send_to_char("Необходимо указать номер или название комнаты.\r\n", ch);
		return;
	}

	if (!*command)
	{
		send_to_char("Что вы собираетесь там делать?\r\n", ch);
		return;
	}

	if ((location = find_target_room(ch, buf, 0)) == NOWHERE)
		return;

	// a location has been found.
	original_loc = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, location);
	command_interpreter(ch, command);

	// check if the char is still there
	if (ch->in_room == location)
	{
		char_from_room(ch);
		char_to_room(ch, original_loc);
	}
	check_horse(ch);
}

void do_unfreeze(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	/*Формат файл unfreeze.lst
	Первая строка email
	Вторая строка причина по которой разфриз
	Все остальные строки полные имена чаров*/
	//char email[50], reason[50];
	Player t_vict;
	CHAR_DATA *vict;
	char *reason_c; // для функции set_punish, она не умеет принимать тип string :(
	std::string email;
	std::string reason;
	std::string name_buffer;
	ifstream unfreeze_list;
	unfreeze_list.open("../lib/misc/unfreeze.lst", fstream::in);
	if (!unfreeze_list)
	{
		send_to_char("Файл unfreeze.lst отсутствует!\r\n", ch);
		return;
	}
	unfreeze_list >> email;
	unfreeze_list >> reason;
	sprintf(buf, "Начинаем масс.разфриз\r\nEmail:%s\r\nПричина:%s\r\n", email.c_str(), reason.c_str());
	send_to_char(buf, ch);
	reason_c = new char[reason.length() + 1];
	strcpy(reason_c, reason.c_str());

	while (!unfreeze_list.eof())
	{
		unfreeze_list >> name_buffer;
		if (load_char(name_buffer.c_str(), &t_vict) < 0)
		{
			sprintf(buf, "Чара с именем %s не существует !\r\n", name_buffer.c_str());
			send_to_char(buf, ch);
			continue;
		}
		vict = &t_vict;
		if (GET_EMAIL(vict) != email)
		{
			sprintf(buf, "У чара %s другой емайл.\r\n", name_buffer.c_str());
			send_to_char(buf, ch);
			continue;
		}
		set_punish(ch, vict, SCMD_FREEZE, reason_c, 0);
		vict->save_char();
		sprintf(buf, "Чар %s разморожен.\r\n", name_buffer.c_str());
		send_to_char(buf, ch);
	}

	delete[] reason_c;
	unfreeze_list.close();

}

void do_goto(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	room_rnum location;

	if ((location = find_target_room(ch, argument, 0)) == NOWHERE)
		return;

	if (POOFOUT(ch))
		sprintf(buf, "$n %s", POOFOUT(ch));
	else
		strcpy(buf, "$n растворил$u в клубах дыма.");

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	char_from_room(ch);

	char_to_room(ch, location);
	check_horse(ch);

	if (POOFIN(ch))
		sprintf(buf, "$n %s", POOFIN(ch));
	else
		strcpy(buf, "$n возник$q посреди комнаты.");
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
}

void do_teleport(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	room_rnum target;

	two_arguments(argument, buf, buf2);

	if (!*buf)
		send_to_char("Кого вы хотите переместить?\r\n", ch);
	else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else if (victim == ch)
		send_to_char("Используйте 'прыжок' для собственного перемещения.\r\n", ch);
	else if (GET_LEVEL(victim) >= GET_LEVEL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		send_to_char("Попробуйте придумать что-то другое.\r\n", ch);
	else if (!*buf2)
		act("Куда вы хотите $S переместить?", FALSE, ch, 0, victim, TO_CHAR);
	else if ((target = find_target_room(ch, buf2, 0)) != NOWHERE)
	{
		send_to_char(OK, ch);
		act("$n растворил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
		char_from_room(victim);
		char_to_room(victim, target);
		check_horse(victim);
		act("$n появил$u, окутанн$w розовым туманом.", FALSE, victim, 0, 0, TO_ROOM);
		act("$n переместил$g вас!", FALSE, ch, 0, (char *)victim, TO_VICT);
		look_at_room(victim, 0);
	}
}

void do_vnum(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "room") && !is_abbrev(buf, "flag")
		&& !is_abbrev(buf, "существо") && !is_abbrev(buf, "предмет") && !is_abbrev(buf, "флаг") && !is_abbrev(buf, "комната")))
	{
		send_to_char("Usage: vnum { obj | mob | flag | room } <name>\r\n", ch);
		return;
	}
	if ((is_abbrev(buf, "mob")) || (is_abbrev(buf, "существо")))
		if (!vnum_mobile(buf2, ch))
			send_to_char("Нет существа с таким именем.\r\n", ch);

	if ((is_abbrev(buf, "obj")) || (is_abbrev(buf, "предмет")))
		if (!vnum_object(buf2, ch))
			send_to_char("Нет предмета с таким названием.\r\n", ch);

	if ((is_abbrev(buf, "flag")) || (is_abbrev(buf, "флаг")))
		if (!vnum_flag(buf2, ch))
			send_to_char("Нет объектов с таким флагом.\r\n", ch);

	if ((is_abbrev(buf, "room")) || (is_abbrev(buf, "комната")))
		if (!vnum_room(buf2, ch))
			send_to_char("Нет объектов с таким флагом.\r\n", ch);
}



void do_stat_room(CHAR_DATA * ch, const int rnum)
{
	ROOM_DATA *rm = world[ch->in_room];
	int i, found;
	OBJ_DATA *j;
	CHAR_DATA *k;
	if (rnum != 0)
	{
		rm = world[rnum];
	}

	sprintf(buf, "Комната : %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprinttype(rm->sector_type, sector_types, buf2);
	sprintf(buf,
		"Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
		zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), ch->in_room, buf2);
	send_to_char(buf, ch);

	rm->flags_sprint(buf2, ",");
	sprintf(buf, "СпецПроцедура: %s, Флаги: %s\r\n", (rm->func == NULL) ? "None" : "Exists", buf2);
	send_to_char(buf, ch);

	send_to_char("Описание:\r\n", ch);
	send_to_char(RoomDescription::show_desc(rm->description_num), ch);

	if (rm->ex_description)
	{
		sprintf(buf, "Доп. описание:%s", CCCYN(ch, C_NRM));
		for (auto desc = rm->ex_description; desc; desc = desc->next)
		{
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		send_to_char(strcat(buf, "\r\n"), ch);
	}
	sprintf(buf, "Живые существа:%s", CCYEL(ch, C_NRM));
	found = 0;
	size_t counter = 0;
	for (auto k_i = rm->people.begin(); k_i != rm->people.end(); ++k_i)
	{
		const auto k = *k_i;
		++counter;

		if (!CAN_SEE(ch, k))
		{
			continue;
		}
		sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
			(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
		strcat(buf, buf2);
		if (strlen(buf) >= 62)
		{
			if (counter != rm->people.size())
			{
				send_to_char(strcat(buf, ",\r\n"), ch);
			}
			else
			{
				send_to_char(strcat(buf, "\r\n"), ch);
			}
			*buf = found = 0;
		}
	}

	if (*buf)
	{
		send_to_char(strcat(buf, "\r\n"), ch);
	}
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (rm->contents)
	{
		sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
		for (found = 0, j = rm->contents; j; j = j->get_next_content())
		{
			if (!CAN_SEE_OBJ(ch, j))
				continue;
			sprintf(buf2, "%s %s", found++ ? "," : "", j->get_short_description().c_str());
			strcat(buf, buf2);
			if (strlen(buf) >= 62)
			{
				if (j->get_next_content())
				{
					send_to_char(strcat(buf, ",\r\n"), ch);
				}
				else
				{
					send_to_char(strcat(buf, "\r\n"), ch);
				}
				*buf = found = 0;
			}
		}

		if (*buf)
		{
			send_to_char(strcat(buf, "\r\n"), ch);
		}
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		if (rm->dir_option[i])
		{
			if (rm->dir_option[i]->to_room() == NOWHERE)
				sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			else
				sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
					GET_ROOM_VNUM(rm->dir_option[i]->to_room()), CCNRM(ch, C_NRM));
			sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
			sprintf(buf,
				"Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s (%s), Тип: %s\r\n",
				CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
				rm->dir_option[i]->key,
				rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
				rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", buf2);
			send_to_char(buf, ch);
			if (!rm->dir_option[i]->general_description.empty())
			{
                                sprintf(buf, "  %s\r\n", rm->dir_option[i]->general_description.c_str());
			}
			else
			{
				strcpy(buf, "  Нет описания выхода.\r\n");
			}
			send_to_char(buf, ch);
		}
	}

	if (!rm->affected.empty())
	{
		sprintf(buf1, " Аффекты на комнате:\r\n");
		for (const auto& aff : rm->affected)
		{
			sprintf(buf1 + strlen(buf1), "       Заклинание \"%s\" (%d) - %s.\r\n",
				spell_name(aff->type),
				aff->duration,
				((k = find_char(aff->caster_id))
					? GET_NAME(k)
					: "неизвестно"));
		}
		send_to_char(buf1, ch);
	}
	// check the room for a script
	do_sstat_room(rm, ch);
}

void do_stat_object(CHAR_DATA * ch, OBJ_DATA * j, const int virt)
{
	int i, found;
	obj_vnum rnum, vnum;
	OBJ_DATA *j2;
	long int li;
	bool is_grgod = (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO)) ? true : false;

	vnum = GET_OBJ_VNUM(j);
	rnum = GET_OBJ_RNUM(j);
	sprintf(buf, "Название: '%s%s%s',\r\nСинонимы: '&c%s&n',",
		CCYEL(ch, C_NRM),
		(!j->get_short_description().empty() ? j->get_short_description().c_str() : "<None>"),
		CCNRM(ch, C_NRM),
		j->get_aliases().c_str());
	send_to_char(buf, ch);
	if (j->get_custom_label() && j->get_custom_label()->label_text)
	{
		sprintf(buf, " нацарапано: '&c%s&n',", j->get_custom_label()->label_text);
		send_to_char(buf, ch);
	}
	sprintf(buf, "\r\n");
	send_to_char(buf, ch);
	sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
	if (rnum >= 0)
	{
		strcpy(buf2, (obj_proto.func(j->get_rnum()) ? "Есть" : "Нет"));
	}
	else
	{
		strcpy(buf2, "None");
	}

	send_to_char(ch, "VNum: [%s%5d%s], RNum: [%5d], UID: [%d], ID: [%ld]\r\n",
		CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), GET_OBJ_UID(j), j->get_id());

	send_to_char(ch, "Расчет критерия: %f, мортов: (%f) \r\n", j->show_koef_obj(), j->show_mort_req());
	send_to_char(ch, "Тип: %s, СпецПроцедура: %s", buf1, buf2);

	if (GET_OBJ_OWNER(j))
	{
		send_to_char(ch, ", Владелец : %s", get_name_by_unique(GET_OBJ_OWNER(j)));
	}
	if (GET_OBJ_ZONE(j))
		send_to_char(ch, ", Принадлежит зоне VNUM : %d", zone_table[GET_OBJ_ZONE(j)].number);
	if (GET_OBJ_MAKER(j))
	{
		send_to_char(ch, ", Создатель : %s", get_name_by_unique(GET_OBJ_MAKER(j)));
	}
	if (GET_OBJ_PARENT(j))
	{
		send_to_char(ch, ", Родитель(VNum) : [%d]", GET_OBJ_PARENT(j));
	}
	if (GET_OBJ_CRAFTIMER(j) > 0)
	{
		send_to_char(ch, ", &Yскрафчена с таймером : [%d]&n", GET_OBJ_CRAFTIMER(j));
	}
	if (j->get_is_rename()) // изменены падежи
	{
		send_to_char(ch, ", &Gпадежи отличны от прототипа&n");
	}
	sprintf(buf, "\r\nL-Des: %s\r\n%s",
		!j->get_description().empty() ? j->get_description().c_str() : "Нет",
		CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	if (j->get_ex_description())
	{
		sprintf(buf, "Экстра описание:%s", CCCYN(ch, C_NRM));
		for (auto desc = j->get_ex_description(); desc; desc = desc->next)
		{
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		send_to_char(strcat(buf, "\r\n"), ch);
	}
	send_to_char("Может быть надет : ", ch);
	sprintbit(j->get_wear_flags(), wear_bits, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	sprintf(buf, "Материал : ");
	sprinttype(j->get_material(), material_name, buf + strlen(buf));
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Неудобства : ", ch);
	j->get_no_flags().sprintbits(no_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Запреты : ", ch);
	j->get_anti_flags().sprintbits(anti_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Устанавливает аффекты : ", ch);
	j->get_affect_flags().sprintbits(weapon_affects, buf, ",", 4);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Дополнительные флаги  : ", ch);
	GET_OBJ_EXTRA(j).sprintbits(extra_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	sprintf(buf, "Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, ",
		GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTEQ(j), GET_OBJ_RENT(j));
	send_to_char(buf, ch);
	if (check_unlimited_timer(j))
		sprintf(buf, "Таймер: нерушимо\r\n");
	else
		sprintf(buf, "Таймер: %d\r\n", j->get_timer());
	send_to_char(buf, ch);

	auto room = get_room_where_obj(j);
	strcpy(buf, "Находится в комнате : ");
	if (room == NOWHERE || !is_grgod)
	{
		strcat(buf, "нигде");
	}
	else
	{
		sprintf(buf2, "%d", room);
		strcat(buf, buf2);
	}

	strcat(buf, ", В контейнере: ");
	if (j->get_in_obj() && is_grgod)
	{
		sprintf(buf2, "[%d] %s", GET_OBJ_VNUM(j->get_in_obj()), j->get_in_obj()->get_short_description().c_str());
		strcat(buf, buf2);
	}
	else
	{
		strcat(buf, "Нет");
	}

	strcat(buf, ", В инвентаре: ");
	if (j->get_carried_by() && is_grgod)
	{
		strcat(buf, GET_NAME(j->get_carried_by()));
	}
	else if (j->get_in_obj() && j->get_in_obj()->get_carried_by() && is_grgod)
	{
		strcat(buf, GET_NAME(j->get_in_obj()->get_carried_by()));
	}
	else
	{
		strcat(buf, "Нет");
	}

	strcat(buf, ", Надет: ");
	if (j->get_worn_by() && is_grgod)
	{
		strcat(buf, GET_NAME(j->get_worn_by()));
	}
	else if (j->get_in_obj() && j->get_in_obj()->get_worn_by() && is_grgod)
	{
		strcat(buf, GET_NAME(j->get_in_obj()->get_worn_by()));
	}
	else
	{
		strcat(buf, "Нет");
	}

	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	switch (GET_OBJ_TYPE(j))
	{
	case OBJ_DATA::ITEM_LIGHT:
		if (GET_OBJ_VAL(j, 2) < 0)
		{
			strcpy(buf, "Вечный свет!");
		}
		else
		{
			sprintf(buf, "Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
		}
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		sprintf(buf, "Заклинания: (Уровень %d) %s, %s, %s",
			GET_OBJ_VAL(j, 0),
			spell_name(GET_OBJ_VAL(j, 1)),
			spell_name(GET_OBJ_VAL(j, 2)),
			spell_name(GET_OBJ_VAL(j, 3)));
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		sprintf(buf, "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
			spell_name(GET_OBJ_VAL(j, 3)),
			GET_OBJ_VAL(j, 0),
			GET_OBJ_VAL(j, 2),
			GET_OBJ_VAL(j, 1));
		break;

	case OBJ_DATA::ITEM_WEAPON:
		sprintf(buf, "Повреждения: %dd%d, Тип повреждения: %d",
			GET_OBJ_VAL(j, 1),
			GET_OBJ_VAL(j, 2),
			GET_OBJ_VAL(j, 3));
		break;

	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		sprintf(buf, "AC: [%d]  Броня: [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;

	case OBJ_DATA::ITEM_TRAP:
		sprintf(buf, "Spell: %d, - Hitpoints: %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
		//sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Труп: %s",
		//	GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2), YESNO(GET_OBJ_VAL(j, 3)));
		if (IS_CORPSE(j))
		{
			sprintf(buf, "Объем: %d, Тип ключа: %s, VNUM моба: %d, Труп: да",
				GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2));
		}
		else
		{
			sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Сложность замка: %d",
				GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		}
		break;

	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
		{
			std::string spells = drinkcon::print_spells(ch, j);
			boost::trim(spells);
			sprintf(buf, "Обьем: %d, Содержит: %d, Таймер (если 1 отравлено): %d, Жидкость: %s\r\n%s",
				GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3), buf2, spells.c_str());
		}
		break;

	case OBJ_DATA::ITEM_NOTE:
		sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
		break;

	case OBJ_DATA::ITEM_KEY:
		strcpy(buf, "");
		break;

	case OBJ_DATA::ITEM_FOOD:
		sprintf(buf, "Насыщает(час): %d, Таймер (если 1 отравлено): %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 3));
		break;

	case OBJ_DATA::ITEM_MONEY:
		sprintf(buf, "Сумма: %d\r\nВалюта: %s", GET_OBJ_VAL(j, 0),
			GET_OBJ_VAL(j, 1) == currency::GOLD ? "куны" :
			GET_OBJ_VAL(j, 1) == currency::ICE ? "искристые снежинки" :
			"что-то другое"
		);
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		sprintbit(GET_OBJ_SKILL(j), ingradient_bits, buf2);
		sprintf(buf, "%s\r\n", buf2);
		send_to_char(buf, ch);

		if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_USES))
		{
			sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(j, 2));
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_LAG))
		{
			sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(j, 0) & 0xFF));
			if (GET_OBJ_VAL(j, 3) == 0 || GET_OBJ_VAL(j, 3) + i < time(NULL))
				strcat(buf, "(можно применять).\r\n");
			else
			{
				li = GET_OBJ_VAL(j, 3) + i - time(NULL);
				sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
			}
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(j), ITEM_CHECK_LEVEL))
		{
			sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(j, 0) >> 8) & 0x1F);
			send_to_char(buf, ch);
		}

		if ((i = real_object(GET_OBJ_VAL(j, 1))) >= 0)
		{
			sprintf(buf, "прототип %s%s%s.\r\n",
				CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;
	case OBJ_DATA::ITEM_MAGIC_CONTAINER:
	case OBJ_DATA::ITEM_MAGIC_ARROW:
		sprintf(buf, "Заклинание: [%s]. Объем [%d]. Осталось стрел[%d].",
			spell_name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
		break;

	default:
		sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
			GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	}
	send_to_char(strcat(buf, "\r\n"), ch);

	// * I deleted the "equipment status" code from here because it seemed
	// * more or less useless and just takes up valuable screen space.

	if (j->get_contains())
	{
		sprintf(buf, "\r\nСодержит:%s", CCGRN(ch, C_NRM));
		for (found = 0, j2 = j->get_contains(); j2; j2 = j2->get_next_content())
		{
			sprintf(buf2, "%s %s", found++ ? "," : "", j2->get_short_description().c_str());
			strcat(buf, buf2);
			if (strlen(buf) >= 62)
			{
				if (j2->get_next_content())
				{
					send_to_char(strcat(buf, ",\r\n"), ch);
				}
				else
				{
					send_to_char(strcat(buf, "\r\n"), ch);
				}
				*buf = found = 0;
			}
		}

		if (*buf)
		{
			send_to_char(strcat(buf, "\r\n"), ch);
		}
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	found = 0;
	send_to_char("Аффекты:", ch);
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (j->get_affected(i).modifier)
		{
			sprinttype(j->get_affected(i).location, apply_types, buf2);
			sprintf(buf, "%s %+d to %s", found++ ? "," : "", j->get_affected(i).modifier, buf2);
			send_to_char(buf, ch);
		}
	}
	if (!found)
	{
		send_to_char(" Нет", ch);
	}

	if (j->has_skills())
	{
		CObjectPrototype::skills_t skills;
		j->get_skills(skills);
		int skill_num;
		int percent;

		send_to_char("\r\nУмения :", ch);
		for (const auto& it : skills)
		{
			skill_num = it.first;
			percent = it.second;

			if (percent == 0) // TODO: такого не должно быть?
			{
				continue;
			}

			sprintf(buf, " %+d%% to %s", percent, skill_info[skill_num].name);
			send_to_char(buf, ch);
		}
	}
	send_to_char("\r\n", ch);

	if (j->get_ilevel() > 0)
	{
		send_to_char(ch, "Уровень (ilvl): %f\r\n", j->get_ilevel());
	}

	if (j->get_minimum_remorts() != 0)
	{
		send_to_char(ch, "Проставлено поле перевоплощений: %d\r\n", j->get_minimum_remorts());
	}
	else if (j->get_auto_mort_req() > 0)
	{
		send_to_char(ch, "Вычислено поле минимальных перевоплощений: %d\r\n", j->get_auto_mort_req());
	}

	if (is_grgod)
	{
		sprintf(buf, "Сейчас в мире : %d. На постое : %d. Макс в мире: %d\r\n",
			rnum >= 0 ? obj_proto.number(rnum) - (virt ? 1 : 0) : -1, rnum >= 0 ? obj_proto.stored(rnum) : -1, GET_OBJ_MIW(j));
		send_to_char(buf, ch);
		// check the object for a script
		do_sstat_object(ch, j);
	}
}


void do_stat_character(CHAR_DATA * ch, CHAR_DATA * k, const int virt)
{
	int i, i2, found = 0;
	OBJ_DATA *j;
	struct follow_type *fol;

	int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
	int k_room = -1;
	if (!virt && (god_level == LVL_IMPL || (god_level == LVL_GRGOD && !IS_NPC(k))))
	{
		k_room = GET_ROOM_VNUM(IN_ROOM(k));
	}
	sprinttype(to_underlying(GET_SEX(k)), genders, buf);
	if (IS_NPC(k))
	{
		sprinttype(GET_RACE(k) - NPC_RACE_BASIC, npc_race_types, buf2);
		sprintf(buf, "%s %s", buf, buf2);
	}
	sprintf(buf2, " %s '%s' IDNum: [%ld] В комнате [%d] Текущий ID:[%ld]",
		(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
		GET_NAME(k), GET_IDNUM(k), k_room, GET_ID(k));
	send_to_char(strcat(buf, buf2), ch);
	send_to_char(ch, " ЛАГ: [%d]\r\n", k->get_wait());
	if (IS_MOB(k))
	{
		sprintf(buf, "Синонимы: &S%s&s, VNum: [%5d], RNum: [%5d]\r\n", k->get_pc_name().c_str(), GET_MOB_VNUM(k), GET_MOB_RNUM(k));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Падежи: %s/%s/%s/%s/%s/%s ", GET_PAD(k, 0), GET_PAD(k, 1), GET_PAD(k, 2), GET_PAD(k, 3), GET_PAD(k, 4), GET_PAD(k, 5));
	send_to_char(buf, ch);


	if (!IS_NPC(k))
	{

		if (!NAME_GOD(k))
		{
			sprintf(buf, "Имя никем не одобрено!\r\n");
			send_to_char(buf, ch);
		}
		else if (NAME_GOD(k) < 1000)
		{
			sprintf(buf, "Имя запрещено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
			send_to_char(buf, ch);
		}
		else
		{
			sprintf(buf, "Имя одобрено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
			send_to_char(buf, ch);
		}

		sprintf(buf, "Вероисповедание: %s\r\n", religion_name[(int)GET_RELIGION(k)][(int)GET_SEX(k)]);
		send_to_char(buf, ch);

		std::string file_name = GET_NAME(k);
		CreateFileName(file_name);
		sprintf(buf, "E-mail: &S%s&s Unique: %d File: %s\r\n", GET_EMAIL(k), GET_UNIQUE(k), file_name.c_str());
		send_to_char(buf, ch);

		std::string text = RegisterSystem::show_comment(GET_EMAIL(k));
		if (!text.empty())
			send_to_char(ch, "Registered by email from %s\r\n", text.c_str());

		if (GET_REMORT(k))
		{
			sprintf(buf, "Перевоплощений: %d\r\n", GET_REMORT(k));
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_FROZEN) && FREEZE_DURATION(k))
		{
			sprintf(buf, "Заморожен : %ld час [%s].\r\n",
				static_cast<long>((FREEZE_DURATION(k) - time(NULL)) / 3600),
				FREEZE_REASON(k) ? FREEZE_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_HELLED) && HELL_DURATION(k))
		{
			sprintf(buf, "Находится в темнице : %ld час [%s].\r\n",
				static_cast<long>((HELL_DURATION(k) - time(NULL)) / 3600),
				HELL_REASON(k) ? HELL_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_NAMED) && NAME_DURATION(k))
		{
			sprintf(buf, "Находится в комнате имени : %ld час.\r\n",
				static_cast<long>((NAME_DURATION(k) - time(NULL)) / 3600));
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_MUTE) && MUTE_DURATION(k))
		{
			sprintf(buf, "Будет молчать : %ld час [%s].\r\n",
				static_cast<long>((MUTE_DURATION(k) - time(NULL)) / 3600),
				MUTE_REASON(k) ? MUTE_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_DUMB) && DUMB_DURATION(k))
		{
			sprintf(buf, "Будет нем : %ld мин [%s].\r\n",
				static_cast<long>((DUMB_DURATION(k) - time(NULL)) / 60),
				DUMB_REASON(k) ? DUMB_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (!PLR_FLAGGED(k, PLR_REGISTERED) && UNREG_DURATION(k))
		{
			sprintf(buf, "Не будет зарегестрирован : %ld час [%s].\r\n",
				static_cast<long>((UNREG_DURATION(k) - time(NULL)) / 3600),
				UNREG_REASON(k) ? UNREG_REASON(k) : "-");
			send_to_char(buf, ch);
		}

		if (GET_GOD_FLAG(k, GF_GODSLIKE) && GCURSE_DURATION(k))
		{
			sprintf(buf, "Под защитой Богов : %ld час.\r\n",
				static_cast<long>((GCURSE_DURATION(k) - time(NULL)) / 3600));
			send_to_char(buf, ch);
		}
		if (GET_GOD_FLAG(k, GF_GODSCURSE) && GCURSE_DURATION(k))
		{
			sprintf(buf, "Проклят Богами : %ld час.\r\n",
				static_cast<long>((GCURSE_DURATION(k) - time(NULL)) / 3600));
			send_to_char(buf, ch);
		}
	}

	sprintf(buf, "Титул: %s\r\n", (k->player_data.title != "" ? k->player_data.title.c_str() : "<Нет>"));
	send_to_char(buf, ch);
	if (IS_NPC(k))
		sprintf(buf, "L-Des: %s", (k->player_data.long_descr != "" ? k->player_data.long_descr.c_str() : "<Нет>\r\n"));
	else
		sprintf(buf, "L-Des: %s", (k->player_data.description != "" ? k->player_data.description.c_str() : "<Нет>\r\n"));
	send_to_char(buf, ch);

	if (!IS_NPC(k))
	{
		sprinttype(k->get_class(), pc_class_types, buf2);
		sprintf(buf, "Племя: %s, Род: %s, Профессия: %s",
			PlayerRace::GetKinNameByNum(GET_KIN(k), GET_SEX(k)).c_str(),
			k->get_race_name().c_str(), buf2);
		send_to_char(buf, ch);
	}
	else
	{
		std::string str;
		if (k->get_role_bits().any())
		{
			print_bitset(k->get_role_bits(), npc_role_types, ",", str);
		}
		else
		{
			str += "нет";
		}
		send_to_char(ch, "Роли NPC: %s%s%s", CCCYN(ch, C_NRM), str.c_str(), CCNRM(ch, C_NRM));
	}

	char tmp_buf[256];
	if (k->get_zone_group() > 1)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), " : групповой %ldx%d",
			GET_EXP(k) / k->get_zone_group(), k->get_zone_group());
	}
	else
	{
		tmp_buf[0] = '\0';
	}

	sprintf(buf, ", Уровень: [%s%2d%s], Опыт: [%s%10ld%s]%s, Наклонности: [%4d]\r\n",
		CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
		GET_EXP(k), CCNRM(ch, C_NRM), tmp_buf, GET_ALIGNMENT(k));

	send_to_char(buf, ch);

	if (!IS_NPC(k))
	{
		if (CLAN(k))
		{
			send_to_char(ch, "Статус дружины: %s\r\n", GET_CLAN_STATUS(k));
		}

		//added by WorM когда статишь файл собсно показывалось текущее время а не время последнего входа
		time_t ltime = get_lastlogon_by_unique(GET_UNIQUE(k));
		strftime(buf1, sizeof(buf1), "%d-%m-%Y", localtime(&(k->player_data.time.birth)));
		strftime(buf2, sizeof(buf2), "%d-%m-%Y", localtime(&ltime));
		buf1[10] = buf2[10] = '\0';

		sprintf(buf,
			"Создан: [%s] Последний вход: [%s] Играл: [%dh %dm] Возраст: [%d]\r\n",
			buf1, buf2, k->player_data.time.played / 3600, ((k->player_data.time.played % 3600) / 60), age(k)->year);
		send_to_char(buf, ch);

		k->add_today_torc(0);
		sprintf(buf, "Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld), Гривны: %d/%d/%d %d",
			GET_LOADROOM(k), k->get_gold(), k->get_bank(), k->get_total_gold(),
			k->get_ext_money(ExtMoney::TORC_GOLD),
			k->get_ext_money(ExtMoney::TORC_SILVER),
			k->get_ext_money(ExtMoney::TORC_BRONZE),
			k->get_hryvn());

		//. Display OLC zone for immorts .
		if (GET_LEVEL(ch) >= LVL_IMMORT)
		{
			sprintf(buf1, ", %sOLC[%d]%s", CCGRN(ch, C_NRM), GET_OLC_ZONE(k), CCNRM(ch, C_NRM));
			strcat(buf, buf1);
		}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else
	{
		sprintf(buf, "Сейчас в мире : %d. ", GET_MOB_RNUM(k) >= 0 ? mob_index[GET_MOB_RNUM(k)].number - (virt ? 1 : 0) : -1);
		send_to_char(buf, ch);
		std::string stats;
		mob_stat::last_kill_mob(k, stats);
		sprintf(buf, "Последний раз убит: %s", stats.c_str());
		send_to_char(buf, ch);
	}
	sprintf(buf,
		"Сила: [%s%d/%d%s]  Инт : [%s%d/%d%s]  Мудр : [%s%d/%d%s] \r\n"
		"Ловк: [%s%d/%d%s]  Тело:[%s%d/%d%s]  Обаян:[%s%d/%d%s] Размер: [%s%d/%d%s]\r\n",
		CCCYN(ch, C_NRM), k->get_inborn_str(), GET_REAL_STR(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), k->get_inborn_int(), GET_REAL_INT(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), k->get_inborn_wis(), GET_REAL_WIS(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), k->get_inborn_dex(), GET_REAL_DEX(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), k->get_inborn_con(), GET_REAL_CON(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), k->get_inborn_cha(), GET_REAL_CHA(k), CCNRM(ch,
			C_NRM),
		CCCYN(ch, C_NRM), GET_SIZE(k), GET_REAL_SIZE(k), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprintf(buf, "Жизни :[%s%d/%d+%d%s]  Энергии :[%s%d/%d+%d%s]",
		CCGRN(ch, C_NRM), GET_HIT(k), GET_REAL_MAX_HIT(k), hit_gain(k),
		CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k), GET_REAL_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	if (IS_MANA_CASTER(k))
	{
		sprintf(buf, " Мана :[%s%d/%d+%d%s]\r\n",
			CCGRN(ch, C_NRM), GET_MANA_STORED(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM));
	}
	else
	{
		sprintf(buf, "\r\n");
	}
	send_to_char(buf, ch);

	sprintf(buf,
		"Glory: [%d], ConstGlory: [%d], AC: [%d/%d(%d)], Броня: [%d], Попадания: [%2d/%2d/%d], Повреждения: [%2d/%2d/%d]\r\n",
		Glory::get_glory(GET_UNIQUE(k)), GloryConst::get_glory(GET_UNIQUE(k)), GET_AC(k), GET_REAL_AC(k),
		compute_armor_class(k), GET_ARMOUR(k), GET_HR(k),
		GET_REAL_HR(k), GET_REAL_HR(k) + str_bonus(GET_REAL_STR(k), STR_TO_HIT),
		GET_DR(k), GET_REAL_DR(k), GET_REAL_DR(k) + str_bonus(GET_REAL_STR(k), STR_TO_DAM));
	send_to_char(buf, ch);
	sprintf(buf,
		"Защитн.аффекты: [Para:%d/Breath:%d/Spell:%d/Basic:%d], Поглощ: [%d], Стойк: [%d], Реакц: [%d], Воля: [%d]\r\n",
		GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3),
		GET_ABSORBE(k), GET_REAL_SAVING_STABILITY(k), GET_REAL_SAVING_REFLEX(k), GET_REAL_SAVING_WILL(k));
	send_to_char(buf, ch);
	sprintf(buf,
		"Резисты: [Огонь:%d/Воздух:%d/Вода:%d/Земля:%d/Жизнь:%d/Разум:%d/Иммунитет:%d/Тьма:%d]\r\n",
		GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
		GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6), GET_RESIST(k, 7));
	send_to_char(buf, ch);
	sprintf(buf,
		"Защита от маг. аффектов : [%d], Защита от маг. урона : [%d], Защита от физ. урона : [%d]\r\n", GET_AR(k), GET_MR(k), GET_PR(k));
	send_to_char(buf, ch);

	sprintf(buf, "Запом: [%d], УспехКолд: [%d], ВоссЖиз: [%d], ВоссСил: [%d], Поглощ: [%d], Удача: [%d], Иниц: [%d]\r\n",
		GET_MANAREG(k), GET_CAST_SUCCESS(k), GET_HITREG(k), GET_MOVEREG(k), GET_ABSORBE(k), k->calc_morale(), GET_INITIATIVE(k));
	send_to_char(buf, ch);

	sprinttype(GET_POS(k), position_types, buf2);
	sprintf(buf, "Положение: %s, Сражается: %s, Экипирован в металл: %s",
		buf2, (k->get_fighting() ? GET_NAME(k->get_fighting()) : "Нет"), (equip_in_metall(k) ? "Да" : "Нет"));

	if (IS_NPC(k))
	{
		strcat(buf, ", Тип атаки: ");
		strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
	}
	if (k->desc)
	{
		sprinttype(STATE(k->desc), connected_types, buf2);
		strcat(buf, ", Соединение: ");
		strcat(buf, buf2);
	}
	send_to_char(strcat(buf, "\r\n"), ch);

	strcpy(buf, "Позиция по умолчанию: ");
	sprinttype((k->mob_specials.default_pos), position_types, buf2);
	strcat(buf, buf2);

	sprintf(buf2, ", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
	strcat(buf, buf2);
	send_to_char(buf, ch);

	if (IS_NPC(k))
	{
		k->char_specials.saved.act.sprintbits(action_bits, buf2, ",", 4);
		sprintf(buf, "MOB флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		k->mob_specials.npc_flags.sprintbits(function_bits, buf2, ",", 4);
		sprintf(buf, "NPC флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		send_to_char(ch, "Количество атак: %s%d%s. ", CCCYN(ch, C_NRM), k->mob_specials.ExtraAttack + 1, CCNRM(ch, C_NRM));
		send_to_char(ch, "Вероятность использования умений: %s%d%%%s\r\n", CCCYN(ch, C_NRM), k->mob_specials.LikeWork, CCNRM(ch, C_NRM));
		send_to_char(ch, "Умения:&c");
		for (const auto counter : AVAILABLE_SKILLS)
		{
			if (k->get_skill(counter))
			{
				send_to_char(ch, " %s:[%3d]", skill_info[counter].name, k->get_skill(counter));
			}
		}
		send_to_char(ch, "&n\r\n");
	}
	else
	{
		k->char_specials.saved.act.sprintbits(player_bits, buf2, ",", 4);
		sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);

		k->player_specials->saved.pref.sprintbits(preference_bits, buf2, ",", 4);
		sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);

		if (IS_IMPL(ch))
		{
			sprintbitwd(k->player_specials->saved.GodsLike, godslike_bits, buf2, ",");
			sprintf(buf, "GFL: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}

	if (IS_MOB(k))
	{
		sprintf(buf, "Mob СпецПроц: %s, NPC сила удара: %dd%d\r\n",
			(mob_index[GET_MOB_RNUM(k)].func ? "Есть" : "Нет"),
			k->mob_specials.damnodice, k->mob_specials.damsizedice);
		send_to_char(buf, ch);
	}
	sprintf(buf, "Несет - вес %d, предметов %d; ", IS_CARRYING_W(k), IS_CARRYING_N(k));

	for (i = 0, j = k->carrying; j; j = j->get_next_content(), i++) { ; }
	sprintf(buf + strlen(buf), "(в инвентаре) : %d, ", i);

	for (i = 0, i2 = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(k, i))
		{
			i2++;
		}
	}
	sprintf(buf2, "(надето): %d\r\n", i2);
	strcat(buf, buf2);
	send_to_char(buf, ch);

	if (!IS_NPC(k))
	{
		sprintf(buf, "Голод: %d, Жажда: %d, Опьянение: %d\r\n",
			GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
		send_to_char(buf, ch);
	}

	if (god_level >= LVL_GRGOD)
	{
		sprintf(buf, "Ведущий: %s, Ведомые:", (k->has_master() ? GET_NAME(k->get_master()) : "<нет>"));

		for (fol = k->followers; fol; fol = fol->next)
		{
			sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch, 0));
			strcat(buf, buf2);
			if (strlen(buf) >= 62)
			{
				if (fol->next)
					send_to_char(strcat(buf, ",\r\n"), ch);
				else
					send_to_char(strcat(buf, "\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			send_to_char(strcat(buf, "\r\n"), ch);
	}
	// Showing the bitvector
	k->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, ",", 4);
	sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	// Routine to show what spells a char is affected by
	if (!k->affected.empty())
	{
		for (const auto aff : k->affected)
		{
			*buf2 = '\0';
			sprintf(buf, "Заклинания: (%3d%s|%s) %s%-21s%s ", aff->duration + 1,
				(aff->battleflag & AF_PULSEDEC) || (aff->battleflag & AF_SAME_TIME) ? "плс" : "мин",
				(aff->battleflag & AF_BATTLEDEC) || (aff->battleflag & AF_SAME_TIME) ? "рнд" : "мин",
				CCCYN(ch, C_NRM), spell_name(aff->type), CCNRM(ch, C_NRM));
			if (aff->modifier)
			{
				sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int)aff->location]);
				strcat(buf, buf2);
			}
			if (aff->bitvector)
			{
				if (*buf2)
				{
					strcat(buf, ", sets ");
				}
				else
				{
					strcat(buf, "sets ");
				}
				sprintbit(aff->bitvector, affected_bits, buf2);
				strcat(buf, buf2);
			}
			send_to_char(strcat(buf, "\r\n"), ch);
		}
	}

	// check mobiles for a script
	if (IS_NPC(k) && god_level >= LVL_BUILDER)
	{
		do_sstat_character(ch, k);
		if (MEMORY(k))
		{
			struct memory_rec_struct *memchar;
			send_to_char("Помнит:\r\n", ch);
			for (memchar = MEMORY(k); memchar; memchar = memchar->next)
			{
				sprintf(buf, "%10ld - %10ld\r\n",
					static_cast<long>(memchar->id),
					static_cast<long>(memchar->time - time(NULL)));
				send_to_char(buf, ch);
			}
		}
	}
	else  		// this is a PC, display their global variables
	{
		if (SCRIPT(k)->global_vars)
		{
			struct trig_var_data *tv;
			char name[MAX_INPUT_LENGTH];
			void find_uid_name(char *uid, char *name);
			send_to_char("Глобальные переменные:\r\n", ch);
			// currently, variable context for players is always 0, so it is
			// not displayed here. in the future, this might change
			for (tv = k->script->global_vars; tv; tv = tv->next)
			{
				if (*(tv->value) == UID_CHAR)
				{
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [CharUID]: %s\r\n", tv->name, name);
				}
				else if (*(tv->value) == UID_OBJ)
				{
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [ObjUID]: %s\r\n", tv->name, name);
				}
				else if (*(tv->value) == UID_ROOM)
				{
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [RoomUID]: %s\r\n", tv->name, name);
				}
				else
					sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);
				send_to_char(buf, ch);
			}
		}

		std::string quested(k->quested_print());
		if (!quested.empty())
			send_to_char(ch, "Выполнил квесты:\r\n%s\r\n", quested.c_str());

		if (RENTABLE(k))
		{
			sprintf(buf, "Не может уйти на постой %ld\r\n", static_cast<long int>(RENTABLE(k) - time(0)));
			send_to_char(buf, ch);
		}
		if (AGRO(k))
		{
			sprintf(buf, "Агрессор %ld\r\n", static_cast<long int>(AGRO(k) - time(NULL)));
			send_to_char(buf, ch);
		}
		pk_list_sprintf(k, buf);
		send_to_char(buf, ch);
		// Отображаем карму.
		if (KARMA(k))
		{
			sprintf(buf, "Карма:\r\n%s", KARMA(k));
			send_to_char(buf, ch);
		}

	}
}

void do_statip(CHAR_DATA * ch, CHAR_DATA * k)
{
	log("Start logon list stat");

	// Отображаем список ip-адресов с которых персонаж входил
	if (!LOGON_LIST(k).empty())
	{
		// update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
		// заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
		std::ostringstream out("Персонаж заходил с IP-адресов:\r\n");
		for(const auto& logon : LOGON_LIST(k))
		{
			sprintf(buf1, "%16s %5ld %20s%s\r\n", logon.ip, logon.count, rustime(localtime(&logon.lasttime)), logon.is_first ? " (создание)" : "");

			out << buf1;
		}
		page_string(ch->desc, out.str());
	}

	log("End logon list stat");
}

void do_stat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	OBJ_DATA *object;
	int tmp;

	half_chop(argument, buf1, buf2);

	if (!*buf1)
	{
		send_to_char("Состояние КОГО или ЧЕГО?\r\n", ch);
		return;
	}

	int level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);

	if (is_abbrev(buf1, "room") && level >= LVL_BUILDER)
	{
		int vnum, rnum = NOWHERE;
		if (*buf2 && (vnum = atoi(buf2)))
		{
			if ((rnum = real_room(vnum)) != NOWHERE)
				do_stat_room(ch, rnum);
			else
				send_to_char("Состояние какой комнаты?\r\n", ch);
		}
		if (!*buf2)
			do_stat_room(ch);
	}
	else if (is_abbrev(buf1, "mob") && level >= LVL_BUILDER)
	{
		if (!*buf2)
			send_to_char("Состояние какого создания?\r\n", ch);
		else
		{
			if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
				do_stat_character(ch, victim);
			else
				send_to_char("Нет такого создания в этом МАДе.\r\n", ch);
		}
	}
	else if (is_abbrev(buf1, "player"))
	{
		if (!*buf2)
		{
			send_to_char("Состояние какого игрока?\r\n", ch);
		}
		else
		{
			if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
				do_stat_character(ch, victim);
			else
				send_to_char("Этого персонажа сейчас нет в игре.\r\n", ch);
		}
	}
	else if (is_abbrev(buf1, "ip"))
	{
		if (!*buf2)
		{
			send_to_char("Состояние ip какого игрока?\r\n", ch);
		}
		else
		{
			if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
			{
				do_statip(ch, victim);
				return;
			}
			else
			{
				send_to_char("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
			}

			Player t_vict;
			if (load_char(buf2, &t_vict) > -1)
			{
				//Clan::SetClanData(&t_vict); не понял зачем проставлять клановый статус тут?
				do_statip(ch, &t_vict);
			}
			else
			{
				send_to_char("Такого игрока нет ВООБЩЕ.\r\n", ch);
			}
		}
	}
	else if (is_abbrev(buf1, "file"))
	{
		if (!*buf2)
		{
			send_to_char("Состояние какого игрока(из файла)?\r\n", ch);
		}
		else
		{
			Player t_vict;
			if (load_char(buf2, &t_vict) > -1)
			{
				if (GET_LEVEL(&t_vict) > level)
				{
					send_to_char("Извините, вам это еще рано.\r\n", ch);
				}
				else
				{
					Clan::SetClanData(&t_vict);
					do_stat_character(ch, &t_vict);
				}
			}
			else
			{
				send_to_char("Такого игрока нет ВООБЩЕ.\r\n", ch);
			}
		}
	}
	else if (is_abbrev(buf1, "object") && level >= LVL_BUILDER)
	{
		if (!*buf2)
			send_to_char("Состояние какого предмета?\r\n", ch);
		else
		{
			if ((object = get_obj_vis(ch, buf2)) != NULL)
				do_stat_object(ch, object);
			else
				send_to_char("Нет такого предмета в игре.\r\n", ch);
		}
	}
	else
	{
		if (level >= LVL_BUILDER)
		{
			if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != NULL)
				do_stat_object(ch, object);
			else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != NULL)
				do_stat_object(ch, object);
			else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
				do_stat_character(ch, victim);
			else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room]->contents)) != NULL)
				do_stat_object(ch, object);
			else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
				do_stat_character(ch, victim);
			else if ((object = get_obj_vis(ch, buf1)) != NULL)
				do_stat_object(ch, object);
			else
				send_to_char("Ничего похожего с этим именем нет.\r\n", ch);
		}
		else
		{
			if ((victim = get_player_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
				do_stat_character(ch, victim);
			else if ((victim = get_player_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
				do_stat_character(ch, victim);
			else
				send_to_char("Никого похожего с этим именем нет.\r\n", ch);
		}
	}
}

void do_shutdown(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	commands::Shutdown command(ch, argument, shutdown_parameters);
	if (command.parse_arguments())
	{
		command.execute();
	}
}

void stop_snooping(CHAR_DATA * ch)
{
	if (!ch->desc->snooping)
		send_to_char("Вы не подслушиваете.\r\n", ch);
	else
	{
		send_to_char("Вы прекратили подслушивать.\r\n", ch);
		ch->desc->snooping->snoop_by = NULL;
		ch->desc->snooping = NULL;
	}
}

void do_snoop(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim, *tch;

	if (!ch->desc)
		return;

	argument = one_argument(argument, arg);

	if (!*arg)
		stop_snooping(ch);
	else if (!(victim = get_player_vis(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("Нет такого создания в игре.\r\n", ch);
	else if (!victim->desc)
		act("Вы не можете $S подслушать - он$G потерял$G связь..\r\n", FALSE, ch, 0, victim, TO_CHAR);
	else if (victim == ch)
		stop_snooping(ch);
	else if (victim->desc->snooping == ch->desc)
		send_to_char("Вы уже подслушиваете.\r\n", ch);
	else if (victim->desc->snoop_by && victim->desc->snoop_by != ch->desc)
		send_to_char("Дык его уже кто-то из богов подслушивает.\r\n", ch);
	//	else if (!can_snoop(ch, victim))
	//		send_to_char("Дружина данного персонажа находится в состоянии войны с вашей дружиной.\r\n", ch);
	else
	{
		if (victim->desc->original)
			tch = victim->desc->original.get();
		else
			tch = victim;

		const int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
		const int victim_level = PRF_FLAGGED(tch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(tch);

		if (victim_level >= god_level)
		{
			send_to_char("Вы не можете.\r\n", ch);
			return;
		}
		send_to_char(OK, ch);

		ch->desc->snoop_with_map = false;
		if (god_level >= LVL_IMPL && argument && *argument)
		{
			skip_spaces(&argument);
			if (isname(argument, "map") || isname(argument, "карта"))
			{
				ch->desc->snoop_with_map = true;
			}
		}

		if (ch->desc->snooping)
			ch->desc->snooping->snoop_by = NULL;

		ch->desc->snooping = victim->desc;
		victim->desc->snoop_by = ch->desc;
	}
}

void do_switch(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	one_argument(argument, arg);

	if (ch->desc->original)
	{
		send_to_char("Вы уже в чьем-то теле.\r\n", ch);
	}
	else if (!*arg)
	{
		send_to_char("Стать кем?\r\n", ch);
	}
	else
	{
		const auto visible_character = get_char_vis(ch, arg, FIND_CHAR_WORLD);
		if (!visible_character)
		{
			send_to_char("Нет такого создания.\r\n", ch);
		}
		else if (ch == visible_character)
		{
			send_to_char("Вы и так им являетесь.\r\n", ch);
		}
		else if (visible_character->desc)
		{
			send_to_char("Это тело уже под контролем.\r\n", ch);
		}
		else if (!IS_IMPL(ch)
			&& !IS_NPC(visible_character))
		{
			send_to_char("Вы не столь могущественны, чтобы контроолировать тело игрока.\r\n", ch);
		}
		else if (GET_LEVEL(ch) < LVL_GRGOD
			&& ROOM_FLAGGED(IN_ROOM(visible_character), ROOM_GODROOM))
		{
			send_to_char("Вы не можете находиться в той комнате.\r\n", ch);
		}
		else if (!IS_GRGOD(ch)
			&& !Clan::MayEnter(ch, IN_ROOM(visible_character), HCE_PORTAL))
		{
			send_to_char("Вы не сможете проникнуть на частную территорию.\r\n", ch);
		}
		else
		{
			const auto victim = character_list.get_character_by_address(visible_character);
			const auto me = character_list.get_character_by_address(ch);
			if (!victim || !me)
			{
				send_to_char("Something went wrong. Report this bug to developers\r\n", ch);
				return;
			}

			send_to_char(OK, ch);

			ch->desc->character = victim;
			ch->desc->original = me;

			victim->desc = ch->desc;
			ch->desc = NULL;
		}
	}
}

void do_return(CHAR_DATA *ch, char *argument, int cmd, int subcmd)
{
	if (ch->desc && ch->desc->original)
	{
		send_to_char("Вы вернулись в свое тело.\r\n", ch);

		/*
		 * If someone switched into your original body, disconnect them.
		 *   - JE 2/22/95
		 *
		 * Zmey: here we put someone switched in our body to disconnect state
		 * but we must also NULL his pointer to our character, otherwise
		 * close_socket() will damage our character's pointer to our descriptor
		 * (which is assigned below in this function). 12/17/99
		 */
		if (ch->desc->original->desc)
		{
			ch->desc->original->desc->character = NULL;
			STATE(ch->desc->original->desc) = CON_DISCONNECT;
		}
		ch->desc->character = ch->desc->original;
		ch->desc->original = NULL;

		ch->desc->character->desc = ch->desc;
		ch->desc = NULL;
	}
	else
	{
		do_recall(ch, argument, cmd, subcmd);
	}
}

void do_load(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *mob;
	mob_vnum number;
	mob_rnum r_num;
	char *iname;

	iname = two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !a_isdigit(*buf2))
	{
		send_to_char("Usage: load { obj | mob } <number>\r\n"
			"       load ing { <сила> | <VNUM> } <имя>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0)
	{
		send_to_char("Отрицательный моб опасен для вашего здоровья!\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "mob"))
	{
		if ((r_num = real_mobile(number)) < 0)
		{
			send_to_char("Нет такого моба в этом МУДе.\r\n", ch);
			return;
		}
		if ((zone_table[real_zone(number)].locked) && (GET_LEVEL(ch) != LVL_IMPL))
		{
			send_to_char("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		char_to_room(mob, ch->in_room);
		act("$n порыл$u в МУДе.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n создал$g $N3!", FALSE, ch, 0, mob, TO_ROOM);
		act("Вы создали $N3.", FALSE, ch, 0, mob, TO_CHAR);
		load_mtrigger(mob);
		olc_log("%s load mob %s #%d", GET_NAME(ch), GET_NAME(mob), number);
	}
	else if (is_abbrev(buf, "obj"))
	{
		if ((r_num = real_object(number)) < 0)
		{
			send_to_char("Господи, да изучи ты номера объектов.\r\n", ch);
			return;
		}
		if ((zone_table[real_zone(number)].locked) && (GET_LEVEL(ch) != LVL_IMPL))
		{
			send_to_char("Зона защищена от записи. С вопросами к старшим богам.\r\n", ch);
			return;
		}
		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		obj->set_crafter_uid(GET_UNIQUE(ch));

		if (number == GlobalDrop::MAGIC1_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC2_ENCHANT_VNUM
			|| number == GlobalDrop::MAGIC3_ENCHANT_VNUM)
		{
			generate_magic_enchant(obj.get());
		}

		if (load_into_inventory)
		{
			obj_to_char(obj.get(), ch);
		}
		else
		{
			obj_to_room(obj.get(), ch->in_room);
		}

		act("$n покопал$u в МУДе.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n создал$g $o3!", FALSE, ch, obj.get(), 0, TO_ROOM);
		act("Вы создали $o3.", FALSE, ch, obj.get(), 0, TO_CHAR);
		load_otrigger(obj.get());
		obj_decay(obj.get());
		olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), number);
	}
	else if (is_abbrev(buf, "ing"))
	{
		int power, i;
		power = atoi(buf2);
		skip_spaces(&iname);
		i = im_get_type_by_name(iname, 0);
		if (i < 0)
		{
			send_to_char("Неверное имя типа\r\n", ch);
			return;
		}
		const auto obj = load_ingredient(i, power, power);
		if (!obj)
		{
			send_to_char("Ошибка загрузки ингредиента\r\n", ch);
			return;
		}
		obj_to_char(obj, ch);
		act("$n покопал$u в МУДе.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n создал$g $o3!", FALSE, ch, obj, 0, TO_ROOM);
		act("Вы создали $o3.", FALSE, ch, obj, 0, TO_CHAR);
		sprintf(buf, "%s load ing %d %s", GET_NAME(ch), power, iname);
		mudlog(buf, NRM, LVL_BUILDER, IMLOG, TRUE);
		load_otrigger(obj);
		obj_decay(obj);
		olc_log("%s load ing %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), power);
	}
	else
	{
		send_to_char("Нет уж. Ты создай че-нить нормальное.\r\n", ch);
	}
}

// отправка сообщения вообще всем
void send_to_all(char * buffer)
{
	DESCRIPTOR_DATA *pt;
	for (pt = descriptor_list; pt; pt = pt->next)
	{
		if (STATE(pt) == CON_PLAYING && pt->character)
		{
			send_to_char(buffer, pt->character.get());
		}
	}
}

void do_vstat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *mob;
	mob_vnum number;	// or obj_vnum ...
	mob_rnum r_num;		// or obj_rnum ...

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !a_isdigit(*buf2))
	{
		send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0)
	{
		send_to_char("Отрицательный номер? Оригинально!\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "mob"))
	{
		if ((r_num = real_mobile(number)) < 0)
		{
			send_to_char("Обратитесь в Арктику - там ОН живет.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		char_to_room(mob, 1);
		do_stat_character(ch, mob, 1);
		extract_char(mob, FALSE);
	}
	else if (is_abbrev(buf, "obj"))
	{
		if ((r_num = real_object(number)) < 0)
		{
			send_to_char("Этот предмет явно перенесли в РМУД.\r\n", ch);
			return;
		}

		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		do_stat_object(ch, obj.get(), 1);
		extract_obj(obj.get());
	}
	else
		send_to_char("Тут должно быть что-то типа 'obj' или 'mob'.\r\n", ch);
}

// clean a room of all mobiles and objects
void do_purge(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;
	OBJ_DATA *obj, *next_o;

	one_argument(argument, buf);

	if (*buf)
	{		// argument supplied. destroy single object or char
		if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)) != NULL)
		{
			if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && !PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				send_to_char("Да я вас за это...\r\n", ch);
				return;
			}
			act("$n обратил$g в прах $N3.", FALSE, ch, 0, vict, TO_NOTVICT);
			if (!IS_NPC(vict))
			{
				sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
				imm_log("%s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				if (vict->desc)
				{
					STATE(vict->desc) = CON_CLOSE;
					vict->desc->character = NULL;
					vict->desc = NULL;
				}
			}
			// TODO: честно говоря дублирование куска из экстракта не ясно
			// смену лидера пока сюду не сую, над вникнуть будет...
			if (vict->followers
				|| vict->has_master())
			{
				die_follower(vict);
			}

			if (!vict->purged())
			{
				extract_char(vict, FALSE);
			}
		}
		else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room]->contents)) != NULL)
		{
			act("$n просто разметал$g $o3 на молекулы.", FALSE, ch, obj, 0, TO_ROOM);
			extract_obj(obj);
		}
		else
		{
			send_to_char("Ничего похожего с таким именем нет.\r\n", ch);
			return;
		}
		send_to_char(OK, ch);
	}
	else  		// no argument. clean out the room
	{
		act("$n произнес$q СЛОВО... вас окружило пламя!", FALSE, ch, 0, 0, TO_ROOM);
		send_to_room("Мир стал немного чище.\r\n", ch->in_room, FALSE);

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy)
		{
			if (IS_NPC(vict))
			{
				if (vict->followers
					|| vict->has_master())
				{
					die_follower(vict);
				}

				if (!vict->purged())
				{
					extract_char(vict, FALSE);
				}
			}
		}

		for (obj = world[ch->in_room]->contents; obj; obj = next_o)
		{
			next_o = obj->get_next_content();
			extract_obj(obj);
		}
	}
}

void send_list_char(std::string list_char, std::string email)
{
	std::string cmd_line = "python3 send_list_char.py " + email + " " + list_char + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

const int IIP = 1;
const int IMAIL = 2;
const int ICHAR = 3;

char *show_pun_time(int time)
{
	static char time_buf[16];
	time_buf[0] = '\0';
	if (time < 3600)
		snprintf(time_buf, sizeof(time_buf), "%d m", (int)time / 60);
	else if (time < 3600 * 24)
		snprintf(time_buf, sizeof(time_buf), "%d h", (int)time / 3600);
	else if (time < 3600 * 24 * 30)
		snprintf(time_buf, sizeof(time_buf), "%d D", (int)time / (3600 * 24));
	else if (time < 3600 * 24 * 365)
		snprintf(time_buf, sizeof(time_buf), "%d M", (int)time / (3600 * 24 * 30));
	else
		snprintf(time_buf, sizeof(time_buf), "%d Y", (int)time / (3600 * 24 * 365));
	return time_buf;
}
//выводим наказания для чара
void show_pun(CHAR_DATA *vict, char *buf)
{
	if (PLR_FLAGGED(vict, PLR_FROZEN)
		&& FREEZE_DURATION(vict))
		sprintf(buf + strlen(buf), "FREEZE : %s [%s].\r\n",
			show_pun_time(FREEZE_DURATION(vict) - time(NULL)),
			FREEZE_REASON(vict) ? FREEZE_REASON(vict)
			: "-");

	if (PLR_FLAGGED(vict, PLR_MUTE)
		&& MUTE_DURATION(vict))
		sprintf(buf + strlen(buf), "MUTE   : %s [%s].\r\n",
			show_pun_time(MUTE_DURATION(vict) - time(NULL)),
			MUTE_REASON(vict) ? MUTE_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, PLR_DUMB)
		&& DUMB_DURATION(vict))
		sprintf(buf + strlen(buf), "DUMB   : %s [%s].\r\n",
			show_pun_time(DUMB_DURATION(vict) - time(NULL)),
			DUMB_REASON(vict) ? DUMB_REASON(vict) : "-");

	if (PLR_FLAGGED(vict, PLR_HELLED)
		&& HELL_DURATION(vict))
		sprintf(buf + strlen(buf), "HELL   : %s [%s].\r\n",
			show_pun_time(HELL_DURATION(vict) - time(NULL)),
			HELL_REASON(vict) ? HELL_REASON(vict) : "-");

	if (!PLR_FLAGGED(vict, PLR_REGISTERED)
		&& UNREG_DURATION(vict))
		sprintf(buf + strlen(buf), "UNREG  : %s [%s].\r\n",
			show_pun_time(UNREG_DURATION(vict) - time(NULL)),
			UNREG_REASON(vict) ? UNREG_REASON(vict) : "-");
}

void inspecting()
{
	if (inspect_list.size() == 0)
		return;

	InspReqListType::iterator it = inspect_list.begin();

	CHAR_DATA *ch = 0;
	DESCRIPTOR_DATA *d_vict = 0;

	//если нет дескриптора или он где-то там по меню шарица, то запрос отменяется
	if (!(d_vict = DescByUID(player_table[it->first].unique))
		|| (STATE(d_vict) != CON_PLAYING)
		|| !(ch = d_vict->character.get()))
	{
		inspect_list.erase(it->first);
		return;
	}

	struct timeval start, stop, result;
	time_t mytime;
	int mail_found = 0;
	int is_online;
	need_warn = false;

	gettimeofday(&start, NULL);
	for (; it->second->pos < static_cast<int>(player_table.size()); it->second->pos++)
	{
		gettimeofday(&stop, NULL);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= OPT_USEC)
		{
			return;
		}

#ifdef TEST_BUILD
		log("inspecting %d/%d", 1 + it->second->pos, player_table.size());
#endif

		if (!*it->second->req)
		{
			send_to_char(ch, "Ошибка: пустой параметр для поиска");//впринципе никогда не должно вылезти, но на всякий случай воткнул проверку
			break;
		}

		if ((it->second->sfor == ICHAR && it->second->unique == player_table[it->second->pos].unique)//Это тот же перс которого мы статим
			|| (player_table[it->second->pos].level >= LVL_IMMORT && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[it->second->pos].level > GET_LEVEL(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO)))//если левел больше то облом
		{
			continue;
		}

		buf1[0] = '\0';
		buf2[0] = '\0';
		is_online = 0;

		CHAR_DATA::shared_ptr vict;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict)
		{
			is_online = 1;
		}

		if (it->second->sfor != IMAIL && it->second->fullsearch)
		{
			if (d_vict)
			{
				vict = d_vict->character;
			}
			else
			{
				vict.reset(new Player);
				if (load_char(player_table[it->second->pos].name(), vict.get()) < 0)
				{
					send_to_char(ch, "Некорректное имя персонажа (%s) inspecting %s: %s.\r\n",
						player_table[it->second->pos].name(), (it->second->sfor == IMAIL ? "mail" : (it->second->sfor == IIP ? "ip" : "char")), it->second->req);
					continue;
				}
			}

			show_pun(vict.get(), buf2);
		}

		if (it->second->sfor == IMAIL || it->second->sfor == ICHAR)
		{
			mail_found = 0;
			if (player_table[it->second->pos].mail)
			{
				if ((it->second->sfor == IMAIL
					&& strstr(player_table[it->second->pos].mail, it->second->req))
					|| (it->second->sfor == ICHAR && !strcmp(player_table[it->second->pos].mail, it->second->mail)))
				{
					mail_found = 1;
				}
			}
		}

		if (it->second->sfor == IIP
			|| it->second->sfor == ICHAR)
		{
			if (!it->second->fullsearch)
			{
				if (player_table[it->second->pos].last_ip)
				{
					if ((it->second->sfor == IIP
						&& strstr(player_table[it->second->pos].last_ip, it->second->req))
						|| (!it->second->ip_log.empty() && !str_cmp(player_table[it->second->pos].last_ip, it->second->ip_log.at(0).ip)))
					{
						sprintf(buf1 + strlen(buf1), " IP:%s%-16s%s\r\n", (it->second->sfor == ICHAR ? CCBLU(ch, C_SPR) : ""), player_table[it->second->pos].last_ip, (it->second->sfor == ICHAR ? CCNRM(ch, C_SPR) : ""));
					}
				}
			}
			else if (vict && !LOGON_LIST(vict).empty())
			{
				for(const auto& cur_log : LOGON_LIST(vict))
				{
					for(const auto& ch_log : it->second->ip_log)
					{
						if (!ch_log.ip)
						{
							send_to_char(ch, "Ошибка: пустой ip\r\n");//поиск прерываеться если криво заполнено поле ip для поиска
							break;
						}

						if ((it->second->sfor == IIP
							&& strstr(cur_log.ip, ch_log.ip))
							|| !str_cmp(cur_log.ip, ch_log.ip))
						{
							sprintf(buf1 + strlen(buf1), " IP:%s%-16s%sCount:%5ld Last: %-30s%s",
								(it->second->sfor == ICHAR ? CCBLU(ch, C_SPR) : ""),
								cur_log.ip,
								(it->second->sfor == ICHAR ? CCNRM(ch, C_SPR) : ""),
								cur_log.count,
								rustime(localtime(&cur_log.lasttime)),
								(it->second->sfor == IIP ? "\r\n" : ""));
							if (it->second->sfor == ICHAR)
							{
								sprintf(buf1 + strlen(buf1), "-> Count:%5ld Last : %s\r\n",
									ch_log.count, rustime(localtime(&ch_log.lasttime)));
							}
						}
					}
				}
			}
		}

		if (*buf1 || mail_found)
		{
			const auto& player = player_table[it->second->pos];
			mytime = player_table[it->second->pos].last_logon;
			sprintf(buf, "Имя: %s%-12s%s e-mail: %s&S%-30s&s%s Last: %s. Level %d/%d.\r\n",
				(is_online ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
				player.name(),
				CCNRM(ch, C_SPR),
				(mail_found && it->second->sfor != IMAIL ? CCBLU(ch, C_SPR) : ""),
				player.mail,
				(mail_found ? CCNRM(ch, C_SPR) : ""),
				rustime(localtime(&mytime)),
				player.level, player.remorts);
			it->second->out += buf;
			it->second->out += buf2;
			it->second->out += buf1;
			it->second->found++;
		}
	}

	need_warn = true;
	gettimeofday(&stop, NULL);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d за %ldсек.\r\n", it->second->found, result.tv_sec);
	it->second->out += buf1;
	if (it->second->sendmail)
		if (it->second->found > 1 && it->second->sfor == IMAIL)
		{
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
void do_inspect(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *d_vict = 0;
	int i = 0;

	if (ch->get_pfilepos() < 0)
		return;

	InspReqListType::iterator it = inspect_list.find(GET_UNIQUE(ch));
	SetAllInspReqListType::iterator it_setall = setall_inspect_list.find(GET_UNIQUE(ch));
	// Навсякий случай разрешаем только одну команду такого типа, либо сетол, либо инспект
	if (it != inspect_list.end() && it_setall != setall_inspect_list.end())
	{
		send_to_char(ch, "Обрабатывается другой запрос, подождите...\r\n");
		return;
	}
	argument = two_arguments(argument, buf, buf2);
	if (!*buf || !*buf2 || !a_isascii(*buf2))
	{
		send_to_char("Usage: inspect { mail | ip | char } <argument> [all|все|sendmail]\r\n", ch);
		return;
	}
	if (!isname(buf, "mail ip char"))
	{
		send_to_char("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if (strlen(buf2) < 3)
	{
		send_to_char("Слишком короткий запрос\r\n", ch);
		return;
	}
	if (strlen(buf2) > 65)
	{
		send_to_char("Слишком длинный запрос\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "char") && (GetUniqueByName(buf2) <= 0))
	{
		send_to_char(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", buf2);
		return;
	}
	InspReqPtr req(new inspect_request);
	req->mail = NULL;
	req->fullsearch = 0;
	req->req = str_dup(buf2);
	req->sendmail = false;
	buf2[0] = '\0';

	if (argument)
	{
		if (isname(argument, "все all"))
			if (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				need_warn = false;
				req->fullsearch = 1;
			}
		if (isname(argument, "sendmail"))
			req->sendmail = true;
	}
	if (is_abbrev(buf, "mail"))
	{
		req->sfor = IMAIL;
	}
	else if (is_abbrev(buf, "ip"))
	{
		req->sfor = IIP;
		if (req->fullsearch)
		{
			const logon_data logon = { str_dup(req->req), 0, 0, false };
			req->ip_log.push_back(logon);
		}
	}
	else if (is_abbrev(buf, "char"))
	{
		req->sfor = ICHAR;
		req->unique = GetUniqueByName(req->req);
		i = get_ptable_by_unique(req->unique);
		if ((req->unique <= 0)//Перс не существует
			|| (player_table[i].level >= LVL_IMMORT && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[i].level > GET_LEVEL(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO)))//если левел больше то облом
		{
			send_to_char(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", req->req);
			req.reset();
			return;
		}

		d_vict = DescByUID(req->unique);
		req->mail = str_dup(player_table[i].mail);
		time_t tmp_time = player_table[i].last_logon;
		sprintf(buf, "Персонаж: %s%s%s e-mail: %s&S%s&s%s Last: %s%s%s from IP: %s%s%s\r\n", (d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
			player_table[i].name(), CCNRM(ch, C_SPR), CCWHT(ch, C_SPR), req->mail, CCNRM(ch, C_SPR),
			CCWHT(ch, C_SPR), rustime(localtime(&tmp_time)), CCNRM(ch, C_SPR),
			CCWHT(ch, C_SPR), player_table[i].last_ip, CCNRM(ch, C_SPR));
		if (req->fullsearch)
		{
			CHAR_DATA::shared_ptr vict;
			if (d_vict)
			{
				vict = d_vict->character;
			}
			else
			{
				vict.reset(new Player);
				if (load_char(req->req, vict.get()) < 0)
				{
					send_to_char(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", req->req);
					return;
				}
			}

			show_pun(vict.get(), buf2);
			if (vict && !LOGON_LIST(vict).empty())
			{
#ifdef TEST_BUILD
				log("filling logon list");
#endif
				for(const auto& cur_log : LOGON_LIST(vict))
				{
					const logon_data logon = { str_dup(cur_log.ip), cur_log.count, cur_log.lasttime, false };
					req->ip_log.push_back(logon);
				}
			}
		}
		else
		{
			const logon_data logon = { str_dup(player_table[i].last_ip), 0, player_table[i].last_logon, false };
			req->ip_log.push_back(logon);
		}
	}

	if (req->sfor < ICHAR)
	{
		sprintf(buf, "%s: %s&S%s&s%s\r\n", (req->sfor == IIP ? "IP" : "e-mail"),
			CCWHT(ch, C_SPR), req->req, CCNRM(ch, C_SPR));
	}
	req->pos = 0;
	req->found = 0;
	req->out += buf;
	req->out += buf2;

	gettimeofday(&req->start, NULL);
	inspect_list[ch->get_pfilepos()] = req;
}

const char *logtypes[] =
{
	"нет", "начальный", "краткий", "нормальный", "полный", "\n"
};

// subcmd - канал
void do_syslog(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int tp;

	if (subcmd < 0 || subcmd > LAST_LOG)
	{
		return;
	}

	tp = GET_LOGS(ch)[subcmd];
	if (tp > 4)
		tp = 4;
	if (tp < 0)
		tp = 0;

	one_argument(argument, arg);

	if (*arg)
	{
		if (GET_LEVEL(ch) == LVL_IMMORT)
			logtypes[2] = "\n";
		else
			logtypes[2] = "краткий";
		if (GET_LEVEL(ch) == LVL_GOD)
			logtypes[4] = "\n";
		else
			logtypes[4] = "полный";
		if ((tp = search_block(arg, logtypes, FALSE)) == -1)
		{
			if (GET_LEVEL(ch) == LVL_IMMORT)
				send_to_char("Формат: syslog { нет | начальный }\r\n", ch);
			else if (GET_LEVEL(ch) == LVL_GOD)
				send_to_char("Формат: syslog { нет | начальный | краткий | нормальный }\r\n", ch);
			else
				send_to_char
				("Формат: syslog { нет | начальный | краткий | нормальный | полный }\r\n", ch);
			return;
		}
		GET_LOGS(ch)[subcmd] = tp;
	}
	sprintf(buf, "Тип вашего лога (%s) сейчас %s.\r\n", runtime_config.logs(static_cast<EOutputStream>(subcmd)).title().c_str(), logtypes[tp]);
	send_to_char(buf, ch);
	return;
}

void do_advance(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *victim;
	char *name = arg, *level = buf2;
	int newlevel, oldlevel;

	two_arguments(argument, name, level);

	if (*name)
	{
		if (!(victim = get_player_vis(ch, name, FIND_CHAR_WORLD)))
		{
			send_to_char("Не найду такого игрока.\r\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("Повысить кого?\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) <= GET_LEVEL(victim) && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char("Нелогично.\r\n", ch);
		return;
	}
	if (!*level || (newlevel = atoi(level)) <= 0)
	{
		send_to_char("Это не похоже на уровень.\r\n", ch);
		return;
	}
	if (newlevel > LVL_IMPL)
	{
		sprintf(buf, "%d - максимальный возможный уровень.\r\n", LVL_IMPL);
		send_to_char(buf, ch);
		return;
	}
	if (newlevel > GET_LEVEL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char("Вы не можете установить уровень выше собственного.\r\n", ch);
		return;
	}
	if (newlevel == GET_LEVEL(victim))
	{
		act("$E и так этого уровня.", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}
	oldlevel = GET_LEVEL(victim);
	if (newlevel < oldlevel)
	{
		send_to_char("Вас окутало облако тьмы.\r\n" "Вы почувствовали себя лишенным чего-то.\r\n", victim);
	}
	else
	{
		act("$n сделал$g несколько странных пасов.\r\n"
			"Вам показалось, будто неземное тепло разлилось по каждой клеточке\r\n"
			"Вашего тела, наполняя его доселе невиданными вами ощущениями.\r\n", FALSE, ch, 0, victim, TO_VICT);
	}

	send_to_char(OK, ch);
	if (newlevel < oldlevel)
	{
		log("(GC) %s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
		imm_log("%s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
	}
	else
	{
		log("(GC) %s has advanced %s to level %d (from %d)",
			GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
		imm_log("%s has advanced %s to level %d (from %d)", GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
	}

	gain_exp_regardless(victim, level_exp(victim, newlevel)
		- GET_EXP(victim));
	victim->save_char();
}

void do_restore(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *vict;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Кого вы хотите восстановить?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else
	{
		// имм с привилегией arena может ресторить только чаров, находящихся с ним на этой же арене
		// плюс исключается ситуация, когда они в одной зоне, но чар не в клетке арены
		if (Privilege::check_flag(ch, Privilege::ARENA_MASTER))
		{
			if (!ROOM_FLAGGED(vict->in_room, ROOM_ARENA) || world[ch->in_room]->zone != world[vict->in_room]->zone)
			{
				send_to_char("Не положено...\r\n", ch);
				return;
			}
		}

		GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
		GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
		if (IS_MANA_CASTER(vict))
		{
			GET_MANA_STORED(vict) = GET_MAX_MANA(vict);
		}
		else
		{
			GET_MEM_COMPLETED(vict) = GET_MEM_TOTAL(vict);
		}
		if (GET_CLASS(vict) == CLASS_WARRIOR)
		{
			struct timed_type wctimed;
			wctimed.skill = SKILL_WARCRY;
			wctimed.time = 0;
			timed_to_char(vict, &wctimed);
		}
		if (IS_GRGOD(ch) && IS_IMMORTAL(vict))
		{
			vict->set_str(25);
			vict->set_int(25);
			vict->set_wis(25);
			vict->set_dex(25);
			vict->set_con(25);
			vict->set_cha(25);
		}
		update_pos(vict);
		affect_from_char(vict, SPELL_DRUNKED);
		GET_DRUNK_STATE(vict) = GET_COND(vict, DRUNK) = 0;
		affect_from_char(vict, SPELL_ABSTINENT);

		//сброс таймеров скиллов и фитов
		while (vict->timed)
			timed_from_char(vict, vict->timed);
		while (vict->timed_feat)
			timed_feat_from_char(vict, vict->timed_feat);

		if (subcmd == SCMD_RESTORE_GOD)
		{
			send_to_char(OK, ch);
			act("Вы были полностью восстановлены $N4!", FALSE, vict, 0, ch, TO_CHAR);
		}
	}
}


void perform_immort_vis(CHAR_DATA * ch)
{
	if (GET_INVIS_LEV(ch) == 0 &&
		!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE) && !AFF_FLAGGED(ch, EAffectFlag::AFF_INVISIBLE) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE))
	{
		send_to_char("Ну вот вас и заметили. Стало ли вам легче от этого?\r\n", ch);
		return;
	}

	SET_INVIS_LEV(ch, 0);
	appear(ch);
	send_to_char("Вы теперь полностью видны.\r\n", ch);
}


void perform_immort_invis(CHAR_DATA * ch, int level)
{
	if (IS_NPC(ch))
	{
		return;
	}

	for (const auto tch : world[ch->in_room]->people)
	{
		if (tch == ch)
		{
			continue;
		}

		if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
		{
			act("Вы вздрогнули, когда $n растворил$u на ваших глазах.", FALSE, ch, 0, tch, TO_VICT);
		}

		if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
		{
			act("$n медленно появил$u из пустоты.", FALSE, ch, 0, tch, TO_VICT);
		}
	}

	SET_INVIS_LEV(ch, level);
	sprintf(buf, "Ваш уровень невидимости - %d.\r\n", level);
	send_to_char(buf, ch);
}

void do_invis(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int level;

	if (IS_NPC(ch))
	{
		send_to_char("Вы не можете сделать этого.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg)
	{
		if (GET_INVIS_LEV(ch) > 0)
			perform_immort_vis(ch);
		else
		{
			if (GET_LEVEL(ch) < LVL_IMPL)
				perform_immort_invis(ch, LVL_IMMORT);
			else
				perform_immort_invis(ch, GET_LEVEL(ch));
		}
	}
	else
	{
		level = MIN(atoi(arg), LVL_IMPL);
		if (level > GET_LEVEL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
			send_to_char("Вы не можете достичь невидимости выше вашего уровня.\r\n", ch);
		else if (GET_LEVEL(ch) < LVL_IMPL && level > LVL_IMMORT && !PRF_FLAGGED(ch, PRF_CODERINFO))
			perform_immort_invis(ch, LVL_IMMORT);
		else if (level < 1)
			perform_immort_vis(ch);
		else
			perform_immort_invis(ch, level);
	}
}

void do_gecho(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *pt;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
	{
		send_to_char("Это, пожалуй, ошибка...\r\n", ch);
	}
	else
	{
		sprintf(buf, "%s\r\n", argument);
		for (pt = descriptor_list; pt; pt = pt->next)
		{
			if (STATE(pt) == CON_PLAYING
				&& pt->character
				&& pt->character.get() != ch)
			{
				send_to_char(buf, pt->character.get());
			}
		}

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		{
			send_to_char(OK, ch);
		}
		else
		{
			send_to_char(buf, ch);
		}
	}
}

void do_poofset(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	char **msg;

	switch (subcmd)
	{
	case SCMD_POOFIN:
		msg = &(POOFIN(ch));
		break;
	case SCMD_POOFOUT:
		msg = &(POOFOUT(ch));
		break;
	default:
		return;
	}

	skip_spaces(&argument);

	if (*msg)
		free(*msg);

	if (!*argument)
		*msg = NULL;
	else
		*msg = str_dup(argument);

	send_to_char(OK, ch);
}

void do_dc(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *d;
	int num_to_dc;
	one_argument(argument, arg);
	if (!(num_to_dc = atoi(arg)))
	{
		send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
		return;
	}
	for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

	if (!d)
	{
		send_to_char("Нет такого соединения.\r\n", ch);
		return;
	}

	if (d->character) //Чтоб не крешило при попытке разъединить незалогиненного
	{
		int victim_level = PRF_FLAGGED(d->character, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(d->character);
		int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
		if (victim_level >= god_level)
		{
			if (!CAN_SEE(ch, d->character))
				send_to_char("Нет такого соединения.\r\n", ch);
			else
				send_to_char("Да уж.. Это не есть праффильная идея...\r\n", ch);
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
		send_to_char("Соединение уже разорвано.\r\n", ch);
	else
	{
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
		send_to_char(buf, ch);
		imm_log("Connect closed by %s.", GET_NAME(ch));
	}
}

void do_wizlock(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int value;
	const char *when;

	one_argument(argument, arg);
	if (*arg)
	{
		value = atoi(arg);
		if (value > LVL_IMPL)
			value = LVL_IMPL; // 34е всегда должны иметь возможность зайти
		if (value < 0 || (value > GET_LEVEL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO)))
		{
			send_to_char("Неверное значение для wizlock.\r\n", ch);
			return;
		}
		circle_restrict = value;
		when = "теперь";
	}
	else
		when = "в настоящее время";

	switch (circle_restrict)
	{
	case 0:
		sprintf(buf, "Игра %s полностью открыта.\r\n", when);
		break;
	case 1:
		sprintf(buf, "Игра %s закрыта для новых игроков.\r\n", when);
		break;
	default:
		sprintf(buf, "Только игроки %d %s и выше могут %s войти в игру.\r\n",
			circle_restrict, desc_count(circle_restrict, WHAT_LEVEL), when);
		break;
	}
	send_to_char(buf, ch);
}

void do_date(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int subcmd)
{
	char *tmstr;
	time_t mytime;
	int d, h, m, s;

	if (subcmd == SCMD_DATE)
	{
		mytime = time(nullptr);
	}
	else
	{
		mytime = shutdown_parameters.get_boot_time();
	}

	tmstr = (char *)asctime(localtime(&mytime));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	if (subcmd == SCMD_DATE)
	{
		sprintf(buf, "Текущее время сервера : %s\r\n", tmstr);
	}
	else
	{
		mytime = time(0) - shutdown_parameters.get_boot_time();
		d = mytime / 86400;
		h = (mytime / 3600) % 24;
		m = (mytime / 60) % 60;
		s = mytime % 60;

		sprintf(buf, "Up since %s: %d day%s, %d:%02d.%02d\r\n", tmstr, d, ((d == 1) ? "" : "s"), h, m, s);
	}

	send_to_char(buf, ch);
}

void do_last(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Кого вы хотите найти?\r\n", ch);
		return;
	}

	Player t_chdata;
	Player *chdata = &t_chdata;
	if (load_char(arg, chdata) < 0)
	{
		send_to_char("Нет такого игрока.\r\n", ch);
		return;
	}
	if (GET_LEVEL(chdata) > GET_LEVEL(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char("Вы не столь уж и божественны для этого.\r\n", ch);
	}
	else
	{
		time_t tmp_time = LAST_LOGON(chdata);
		sprintf(buf, "[%5ld] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
			GET_IDNUM(chdata), (int)GET_LEVEL(chdata),
			kin_abbrevs[(int)GET_KIN(chdata)],
			class_abbrevs[(int)GET_CLASS(chdata)], GET_NAME(chdata),
			GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown", ctime(&tmp_time));
		send_to_char(buf, ch);
	}
}

void do_force(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *i, *next_desc;
	char to_force[MAX_INPUT_LENGTH + 2];

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n принудил$g вас '%s'.", to_force);

	if (!*arg || !*to_force)
	{
		send_to_char("Кого и что вы хотите принудить сделать?\r\n", ch);
	}
	else if (!IS_GRGOD(ch) || (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg)
		&& str_cmp("здесь", arg)))
	{
		const auto vict = get_char_vis(ch, arg, FIND_CHAR_WORLD);
		if (!vict)
		{
			send_to_char(NOPERSON, ch);
		}
		else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Господи, только не это!\r\n", ch);
		}
		else
		{
			char *pstr;
			send_to_char(OK, ch);
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			while ((pstr = strchr(buf, '%')) != NULL)
			{
				pstr[0] = '*';
			}
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			command_interpreter(vict, to_force);
		}
	}
	else if (!str_cmp("room", arg)
		|| !str_cmp("здесь", arg))
	{
		send_to_char(OK, ch);
		sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);
		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy)
		{
			if (!IS_NPC(vict)
				&& GET_LEVEL(vict) >= GET_LEVEL(ch)
				&& !PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				continue;
			}

			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	}
	else  		// force all
	{
		send_to_char(OK, ch);
		sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s forced all to %s", GET_NAME(ch), to_force);

		for (i = descriptor_list; i; i = next_desc)
		{
			next_desc = i->next;

			const auto vict = i->character;
			if (STATE(i) != CON_PLAYING
				|| !vict
				|| (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)
					&& !PRF_FLAGGED(ch, PRF_CODERINFO)))
			{
				continue;
			}

			act(buf1, TRUE, ch, NULL, vict.get(), TO_VICT);
			command_interpreter(vict.get(), to_force);
		}
	}
}

void do_sdemigod(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *d;
	// убираем пробелы
	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("Что Вы хотите сообщить ?\r\n", ch);
		return;
	}
	sprintf(buf1, "&c%s демигодам: '%s'&n\r\n", GET_NAME(ch), argument);

	// проходим по всем чарам и отправляем мессагу
	for (d = descriptor_list; d; d = d->next)
	{
		// Проверяем, в игре ли чар
		if (STATE(d) == CON_PLAYING)
		{
			// Если в игре, то проверяем, демигод ли чар
			// Иммы 34-левела тоже могут смотреть канал
			if ((GET_GOD_FLAG(d->character, GF_DEMIGOD)) || (GET_LEVEL(d->character) == LVL_IMPL))
			{
				// Проверяем пишет ли чар или отправляет письмо
				// А так же на реж сдемигод
				if ((!PLR_FLAGGED(d->character, PLR_WRITING)) &&
					(!PLR_FLAGGED(d->character, PLR_MAILING)) &&
					(!PLR_FLAGGED(d->character, PRF_SDEMIGOD)))
				{
					d->character->remember_add(buf1, Remember::ALL);
					send_to_char(buf1, d->character.get());
				}
			}
		}
	}
}

void do_wiznet(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	DESCRIPTOR_DATA *d;
	char emote = FALSE;
	char bookmark1 = FALSE;
	char bookmark2 = FALSE;
	int level = LVL_GOD;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
	{
		send_to_char
		("Формат: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
			"        wiznet @<level> *<emotetext> | wiz @\r\n", ch);
		return;
	}

	//	if (PRF_FLAGGED(ch, PRF_CODERINFO)) return;

		// Опускаем level для gf_demigod
	if (GET_GOD_FLAG(ch, GF_DEMIGOD))
		level = LVL_IMMORT;

	// использование доп. аргументов
	switch (*argument)
	{
	case '*':
		emote = TRUE;
	case '#':
		// Установить уровень имм канала
		one_argument(argument + 1, buf1);
		if (is_number(buf1))
		{
			half_chop(argument + 1, buf1, argument);
			level = MAX(atoi(buf1), LVL_IMMORT);
			if (level > GET_LEVEL(ch))
			{
				send_to_char("Вы не можете изрекать выше вашего уровня.\r\n", ch);
				return;
			}
		}
		else if (emote)
			argument++;
		break;
	case '@':
		// Обнаруживаем всех кто может (теоретически) нас услышать
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) == CON_PLAYING &&
				(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, GF_DEMIGOD)) &&
				!PRF_FLAGGED(d->character, PRF_NOWIZ) && (CAN_SEE(ch, d->character) || IS_IMPL(ch)))
			{
				if (!bookmark1)
				{
					strcpy(buf1,
						"Боги/привилегированные которые смогут (наверное) вас услышать:\r\n");
					bookmark1 = TRUE;
				}
				sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
				if (PLR_FLAGGED(d->character, PLR_WRITING))
					strcat(buf1, " (пишет)\r\n");
				else if (PLR_FLAGGED(d->character, PLR_MAILING))
					strcat(buf1, " (пишет письмо)\r\n");
				else
					strcat(buf1, "\r\n");
			}
		}
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) == CON_PLAYING &&
				(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, GF_DEMIGOD)) &&
				PRF_FLAGGED(d->character, PRF_NOWIZ) && CAN_SEE(ch, d->character))
			{
				if (!bookmark2)
				{
					if (!bookmark1)
						strcpy(buf1,
							"Боги/привилегированные которые не смогут вас услышать:\r\n");
					else
						strcat(buf1,
							"Боги/привилегированные которые не смогут вас услышать:\r\n");

					bookmark2 = TRUE;
				}
				sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
			}
		}
		send_to_char(buf1, ch);

		return;
	case '\\':
		++argument;
		break;
	default:
		break;
	}
	if (PRF_FLAGGED(ch, PRF_NOWIZ))
	{
		send_to_char("Вы вне игры!\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("Не думаю, что Боги одобрят это.\r\n", ch);
		return;
	}
	if (level != LVL_GOD)
	{
		sprintf(buf1, "%s%s: <%d> %s%s\r\n", GET_NAME(ch),
			emote ? "" : " богам", level, emote ? "<--- " : "", argument);
	}
	else
	{
		sprintf(buf1, "%s%s: %s%s\r\n", GET_NAME(ch), emote ? "" : " богам", emote ? "<--- " : "", argument);
	}
	snprintf(buf2, MAX_STRING_LENGTH, "&c%s&n", buf1);
	Remember::add_to_flaged_cont(Remember::wiznet_, buf2, level);

	// пробегаемся по списку дескрипторов чаров и кто должен - тот услышит богов
	for (d = descriptor_list; d; d = d->next)
	{
		if ((STATE(d) == CON_PLAYING) &&	// персонаж должен быть в игре
			((GET_LEVEL(d->character) >= level) ||	// уровень равным или выше level
			(GET_GOD_FLAG(d->character, GF_DEMIGOD) && level == 31)) &&	// демигоды видят 31 канал
				(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&	// игрок с режимом NOWIZ не видит имм канала
			(!PLR_FLAGGED(d->character, PLR_WRITING)) &&	// пишущий не видит имм канала
			(!PLR_FLAGGED(d->character, PLR_MAILING)))	// отправляющий письмо не видит имм канала
		{
			// отправляем сообщение чару
			snprintf(buf2, MAX_STRING_LENGTH, "%s%s%s",
				CCCYN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM));
			d->character->remember_add(buf2, Remember::ALL);
			// не видино своих мессаг если 'режим repeat'
			if (d != ch->desc
				|| !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))
			{
				send_to_char(buf2, d->character.get());
			}
		}
	}

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	{
		send_to_char(OK, ch);
	}
}

void do_zreset(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	zone_rnum i;
	zone_vnum j;

	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Укажите зону.\r\n", ch);
		return;
	}
	if (*arg == '*')
	{
		for (i = 0; i < static_cast<zone_rnum>(zone_table.size()); i++)
		{
			reset_zone(i);
		}

		send_to_char("Перезагружаю мир.\r\n", ch);
		sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s reset entire world.", GET_NAME(ch));
		return;
	}
	else if (*arg == '.')
	{
		i = world[ch->in_room]->zone;
	}
	else
	{
		j = atoi(arg);
		for (i = 0; i < static_cast<zone_rnum>(zone_table.size()); i++)
		{
			if (zone_table[i].number == j)
			{
				break;
			}
		}
	}

	if (i >= 0 && i < static_cast<zone_rnum>(zone_table.size()))
	{
		reset_zone(i);
		sprintf(buf, "Перегружаю зону %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
		send_to_char(buf, ch);
		sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
		mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
	}
	else
	{
		send_to_char("Нет такой зоны.\r\n", ch);
	}
}

// Функции установки разных наказаний.

// *  General fn for wizcommands of the sort: cmd <player>
void do_wizutil(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *vict;
	long result;
	int times = 0;
	char *reason;
	char num[MAX_INPUT_LENGTH];

	//  one_argument(argument, arg);
	reason = two_arguments(argument, arg, num);

	if (!*arg)
		send_to_char("Для кого?\r\n", ch);
	else if (!(vict = get_player_pun(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("Нет такого игрока.\r\n", ch);
	else if (GET_LEVEL(vict) > GET_LEVEL(ch) && !GET_GOD_FLAG(ch, GF_DEMIGOD) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		send_to_char("А он ведь старше вас....\r\n", ch);
	else if (GET_LEVEL(vict) >= LVL_IMMORT && GET_GOD_FLAG(ch, GF_DEMIGOD))
		send_to_char("А он ведь старше вас....\r\n", ch);
	else
	{
		switch (subcmd)
		{
		case SCMD_REROLL:
			send_to_char("Перегенерирую...\r\n", ch);
			roll_real_abils(vict);
			log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
			imm_log("%s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
			sprintf(buf,
				"Новые параметры: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
				vict->get_inborn_str(), vict->get_inborn_int(), vict->get_inborn_wis(),
				vict->get_inborn_dex(), vict->get_inborn_con(), vict->get_inborn_cha());
			send_to_char(buf, ch);
			break;
		case SCMD_NOTITLE:
			result = PLR_TOG_CHK(vict, PLR_NOTITLE);
			sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
			break;
		case SCMD_SQUELCH:
			break;
		case SCMD_MUTE:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_MUTE, reason, times);
			break;
		case SCMD_DUMB:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_DUMB, reason, times);
			break;
		case SCMD_FREEZE:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_FREEZE, reason, times);
			break;
		case SCMD_HELL:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_HELL, reason, times);
			break;

		case SCMD_NAME:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_NAME, reason, times);
			break;

		case SCMD_REGISTER:
			set_punish(ch, vict, SCMD_REGISTER, reason, 0);
			break;

		case SCMD_UNREGISTER:
			set_punish(ch, vict, SCMD_UNREGISTER, reason, 0);
			break;

		case SCMD_UNAFFECT:
			if (!vict->affected.empty())
			{
				while (!vict->affected.empty())
				{
					vict->affect_remove(vict->affected.begin());
				}
				send_to_char("Яркая вспышка осветила вас!\r\n"
					"Вы почувствовали себя немного иначе.\r\n", vict);
				send_to_char("Все афекты сняты.\r\n", ch);
			}
			else
			{
				send_to_char("Аффектов не было изначально.\r\n", ch);
				return;
			}
			break;
		default:
			log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
			break;
		}
		vict->save_char();
	}
}


// single zone printing fn used by "show zone" so it's not repeated in the
// code 3 times ... -je, 4/6/93

void print_zone_to_buf(char **bufptr, zone_rnum zone)
{
	const size_t BUFFER_SIZE = 1024;
	char tmpstr[BUFFER_SIZE];
	snprintf(tmpstr, BUFFER_SIZE,
		"%3d %-60.60s Уровень: %2d; Type: %-20.20s; Age: %3d; Reset: %3d (%1d)(%1d)\r\n"
		"    Top: %5d %s %s; ResetIdle: %s; Занято: %s; Активность: %.2f; Группа: %2d, Mob-level: %2d; Автор: %s\r\n",
		zone_table[zone].number, zone_table[zone].name,
		zone_table[zone].level, zone_types[zone_table[zone].type].name,
		zone_table[zone].age, zone_table[zone].lifespan,
		zone_table[zone].reset_mode,
		(zone_table[zone].reset_mode == 3) ? (can_be_reset(zone) ? 1 : 0) : (is_empty(zone) ? 1 : 0),
		zone_table[zone].top,
		zone_table[zone].under_construction ? "&GТестовая!&n" : " ",
		zone_table[zone].locked ? "&RРедактирование запрещено!&n" : " ",
		zone_table[zone].reset_idle ? "Y" : "N",
		zone_table[zone].used ? "Y" : "N",
		(double)zone_table[zone].activity / 1000,
		zone_table[zone].group,
		zone_table[zone].mob_level,
		zone_table[zone].author ? zone_table[zone].author : "Не известен.");
	*bufptr = str_add(*bufptr, tmpstr);
}

std::string print_zone_exits(zone_rnum zone)
{
	bool found = false;
	char tmp[128];

	snprintf(tmp, sizeof(tmp),
		"\r\nВыходы из зоны %3d:\r\n", zone_table[zone].number);
	std::string out(tmp);

	for (int n = FIRST_ROOM; n <= top_of_world; n++)
	{
		if (world[n]->zone == zone)
		{
			for (int dir = 0; dir < NUM_OF_DIRS; dir++)
			{
				if (world[n]->dir_option[dir]
					&& world[world[n]->dir_option[dir]->to_room()]->zone != zone
					&& world[world[n]->dir_option[dir]->to_room()]->number > 0)
				{
					snprintf(tmp, sizeof(tmp),
						"  Номер комнаты:%5d Направление:%6s Выход в комнату:%5d\r\n",
						world[n]->number, Dirs[dir],
						world[world[n]->dir_option[dir]->to_room()]->number);
					out += tmp;
					found = true;
				}
			}
		}
	}
	if (!found)
	{
		out += "Выходов из зоны не обнаружено.\r\n";
	}
	return out;
}

std::string print_zone_enters(zone_rnum zone)
{
	bool found = true;
	char tmp[128];

	snprintf(tmp, sizeof(tmp),
		"\r\nВходы в зону %3d:\r\n", zone_table[zone].number);
	std::string out(tmp);

	for (int n = FIRST_ROOM; n <= top_of_world; n++)
	{
		if (world[n]->zone != zone)
		{
			for (int dir = 0; dir < NUM_OF_DIRS; dir++)
			{
				if (world[n]->dir_option[dir]
					&& world[world[n]->dir_option[dir]->to_room()]->zone == zone
					&& world[world[n]->dir_option[dir]->to_room()]->number > 0)
				{
					snprintf(tmp, sizeof(tmp),
						"  Номер комнаты:%5d Направление:%6s Вход в комнату:%5d\r\n",
						world[n]->number, Dirs[dir],
						world[world[n]->dir_option[dir]->to_room()]->number);
					out += tmp;
					found = true;
				}
			}
		}
	}
	if (!found)
	{
		out += "Входов в зону не обнаружено.\r\n";
	}
	return out;
}

namespace
{

	bool sort_by_zone_mob_level(int rnum1, int rnum2)
	{
		return !(zone_table[mob_index[rnum1].zone].mob_level < zone_table[mob_index[rnum2].zone].mob_level);
	}

	void print_mob_bosses(CHAR_DATA *ch, bool lvl_sort)
	{
		std::vector<int> tmp_list;
		for (int i = 0; i <= top_of_mobt; ++i)
		{
			if (mob_proto[i].get_role(MOB_ROLE_BOSS))
			{
				tmp_list.push_back(i);
			}
		}
		if (lvl_sort)
		{
			std::sort(tmp_list.begin(), tmp_list.end(), sort_by_zone_mob_level);
		}

		int cnt = 0;
		std::string out(
			"                          имя моба [ср.уровень мобов в зоне][vnum моба] имя зоны\r\n"
			"--------------------------------------------------------------------------------\r\n");

		for (std::vector<int>::const_iterator i = tmp_list.begin(),
			iend = tmp_list.end(); i != iend; ++i)
		{
			const int mob_rnum = *i;
			std::string zone_name_str = zone_table[mob_index[mob_rnum].zone].name ?
				zone_table[mob_index[mob_rnum].zone].name : "EMPTY";

			const auto mob = mob_proto + mob_rnum;
			const auto vnum = GET_MOB_VNUM(mob);
			out += boost::str(boost::format("%3d %31s [%2d][%6d] %31s\r\n")
				% ++cnt
				% (mob->get_name_str().size() > 31
					? mob->get_name_str().substr(0, 31)
					: mob->get_name_str())
				% zone_table[mob_index[mob_rnum].zone].mob_level
				% vnum
				% (zone_name_str.size() > 31
					? zone_name_str.substr(0, 31)
					: zone_name_str));
		}

		page_string(ch->desc, out);
	}

} // namespace

struct show_struct show_fields[] =
{
	{"nothing", 0},		// 0
	{"zones", LVL_IMMORT},	// 1
	{"player", LVL_IMMORT},
	{"rent", LVL_GRGOD},
	{"stats", LVL_IMMORT},
	{"errors", LVL_IMPL},	// 5
	{"death", LVL_GOD},
	{"godrooms", LVL_GOD},
	{"snoop", LVL_GRGOD},
	{"linkdrop", LVL_GRGOD},
	{"punishment", LVL_IMMORT}, // 10
	{"paths", LVL_GRGOD},
	{"loadrooms", LVL_GRGOD},
	{"skills", LVL_IMPL},
	{"spells", LVL_IMPL},
	{"ban", LVL_IMMORT}, // 15
	{"features", LVL_IMPL},
	{"glory", LVL_IMPL},
	{"crc", LVL_IMMORT},
	{"affectedrooms", LVL_IMMORT},
	{"money", LVL_IMPL}, // 20
	{"expgain", LVL_IMPL},
	{"runes", LVL_IMPL},
	{"mobstat", LVL_IMPL},
	{"bosses", LVL_IMPL},
	{"remort", LVL_IMPL}, // 25
	{"\n", 0}
};

void do_show(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int i, j, l, con;	// i, j, k to specifics?

	zone_rnum zrn;
	zone_vnum zvn;
	char self = 0;
	CHAR_DATA *vict;
	DESCRIPTOR_DATA *d;
	char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], value1[MAX_INPUT_LENGTH];
	// char bf[MAX_EXTEND_LENGTH];
	char *bf = NULL;
	char rem[MAX_INPUT_LENGTH];

	skip_spaces(&argument);

	if (!*argument)
	{
		strcpy(buf, "Опции для показа:\r\n");
		for (j = 0, i = 1; show_fields[i].level; i++)
			if (Privilege::can_do_priv(ch, std::string(show_fields[i].cmd), 0, 2))
				sprintf(buf + strlen(buf), "%-15s%s", show_fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		return;
	}

	strcpy(arg, three_arguments(argument, field, value, value1));

	for (l = 0; *(show_fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, show_fields[l].cmd, strlen(field)))
			break;

	if (!Privilege::can_do_priv(ch, std::string(show_fields[l].cmd), 0, 2))
	{
		send_to_char("Вы не столь могущественны, чтобы узнать это.\r\n", ch);
		return;
	}
	if (!strcmp(value, "."))
		self = 1;
	buf[0] = '\0';
	//bf[0] = '\0';
	switch (l)
	{
	case 1:		// zone
		// tightened up by JE 4/6/93
		if (self)
			print_zone_to_buf(&bf, world[ch->in_room]->zone);
		else if (*value1 && is_number(value) && is_number(value1))
		{
			// хотят зоны в диапазоне увидеть
			int found = 0;
			int zstart = atoi(value);
			int zend = atoi(value1);

			for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
			{
				if (zone_table[zrn].number >= zstart
					&& zone_table[zrn].number <= zend)
				{
					print_zone_to_buf(&bf, zrn);
					found = 1;
				}
			}

			if (!found)
			{
				send_to_char("В заданном диапазоне зон нет.\r\n", ch);
				return;
			}
		}
		else if (*value && is_number(value))
		{
			for (zvn = atoi(value), zrn = 0;
				zone_table[zrn].number != zvn && zrn < static_cast<zone_rnum>(zone_table.size());
				zrn++)
			{
				/* empty loop */
			}

			if (zrn < static_cast<zone_rnum>(zone_table.size()))
			{
				print_zone_to_buf(&bf, zrn);
			}
			else
			{
				send_to_char("Нет такой зоны.\r\n", ch);
				return;
			}
		}
		else if (*value && !strcmp(value, "-g"))
		{
			for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
			{
				if (zone_table[zrn].group > 1)
				{
					print_zone_to_buf(&bf, zrn);
				}
			}
		}
		else if (*value1 && !strcmp(value, "-l") && is_number(value1))
		{
			one_argument(arg, value);
			if (*value && is_number(value))
			{
				// show zones -l x y
				for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
				{
					if (zone_table[zrn].mob_level >= atoi(value1)
						&& zone_table[zrn].mob_level <= atoi(value))
					{
						print_zone_to_buf(&bf, zrn);
					}
				}
			}
			else
			{
				// show zones -l x
				for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
				{
					if (zone_table[zrn].mob_level == atoi(value1))
					{
						print_zone_to_buf(&bf, zrn);
					}
				}
			}
		}
		else
		{
			for (zrn = 0; zrn < static_cast<zone_rnum>(zone_table.size()); zrn++)
			{
				print_zone_to_buf(&bf, zrn);
			}
		}

		page_string(ch->desc, bf, TRUE);
		free(bf);
		break;

	case 2:		// player
		if (!*value)
		{
			send_to_char("Уточните имя.\r\n", ch);
			return;
		}
		if (!(vict = get_player_vis(ch, value, FIND_CHAR_WORLD)))
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
		sprintf(buf, "&WИнформация по игроку %s:&n (", GET_NAME(vict));
		sprinttype(to_underlying(GET_SEX(vict)), genders, buf + strlen(buf));
		sprintf(buf + strlen(buf), ")&n\r\n");
		sprintf(buf + strlen(buf), "Падежи : %s/%s/%s/%s/%s/%s\r\n",
			GET_PAD(vict, 0), GET_PAD(vict, 1), GET_PAD(vict, 2),
			GET_PAD(vict, 3), GET_PAD(vict, 4), GET_PAD(vict, 5));
		if (!NAME_GOD(vict))
		{
			sprintf(buf + strlen(buf), "Имя никем не одобрено!\r\n");
		}
		else if (NAME_GOD(vict) < 1000)
		{
			sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(vict)));
			*buf1 = UPPER(*buf1);
			sprintf(buf + strlen(buf), "Имя запрещено богом %s\r\n", buf1);
		}
		else
		{
			sprintf(buf1, "%s", get_name_by_id(NAME_ID_GOD(vict)));
			*buf1 = UPPER(*buf1);
			sprintf(buf + strlen(buf), "Имя одобрено богом %s\r\n", buf1);
		}
		if (GET_REMORT(vict) < 4)
			sprintf(rem, "Перевоплощений: %d\r\n", GET_REMORT(vict));
		else
			sprintf(rem, "Перевоплощений: 3+\r\n");
		sprintf(buf + strlen(buf), "%s", rem);
		sprintf(buf + strlen(buf), "Уровень: %s\r\n", (GET_LEVEL(vict) < 25 ? "ниже 25" : "25+"));
		sprintf(buf + strlen(buf), "Титул: %s\r\n", (vict->player_data.title != "" ? vict->player_data.title.c_str() : "<Нет>"));
		sprintf(buf + strlen(buf), "Описание игрока:\r\n");
		sprintf(buf + strlen(buf), "%s\r\n", (vict->player_data.description != "" ? vict->player_data.description.c_str() : "<Нет>"));
		send_to_char(buf, ch);
		// Отображаем карму.
		if (KARMA(vict))
		{
			sprintf(buf, "\r\n&WИнформация по наказаниям и поощрениям:&n\r\n%s", KARMA(vict));
			send_to_char(buf, ch);
		}
		break;
	case 3:
		if (!*value)
		{
			send_to_char("Уточните имя.\r\n", ch);
			return;
		}
		Crash_listrent(ch, value);
		break;
	case 4:
	{
		i = 0;
		j = 0;
		con = 0;
		int motion = 0;
		for (const auto vict : character_list)
		{
			if (IS_NPC(vict))
			{
				j++;
			}
			else
			{
				if (vict->is_active())
				{
					++motion;
				}
				if (CAN_SEE(ch, vict))
				{
					i++;
					if (vict->desc)
					{
						con++;
					}
				}
			}
		}

		strcpy(buf, "Текущее состояние:\r\n");
		sprintf(buf + strlen(buf), "  Игроков в игре - %5d, соединений - %5d\r\n", i, con);
		sprintf(buf + strlen(buf), "  Всего зарегистрировано игроков - %5zd\r\n", player_table.size());
		sprintf(buf + strlen(buf), "  Мобов - %5d,  прообразов мобов - %5d\r\n", j, top_of_mobt + 1);
		sprintf(buf + strlen(buf), "  Предметов - %5zd, прообразов предметов - %5zd\r\n",
			world_objects.size(), obj_proto.size());
		sprintf(buf + strlen(buf), "  Комнат - %5d, зон - %5zd\r\n", top_of_world + 1, zone_table.size());
		sprintf(buf + strlen(buf), "  Больших буферов - %5d\r\n", buf_largecount);
		sprintf(buf + strlen(buf), "  Переключенных буферов - %5d, переполненных - %5d\r\n", buf_switches, buf_overflows);
		sprintf(buf + strlen(buf), "  Послано байт - %lu\r\n", number_of_bytes_written);
		sprintf(buf + strlen(buf), "  Получено байт - %lu\r\n", number_of_bytes_read);
		sprintf(buf + strlen(buf), "  Максимальный ID - %ld\r\n", max_id.current());
		sprintf(buf + strlen(buf), "  Активность игроков (cmds/min) - %lu\r\n",
			static_cast<unsigned long>((cmd_cnt * 60) / (time(0) - shutdown_parameters.get_boot_time())));
		send_to_char(buf, ch);
		Depot::show_stats(ch);
		Glory::show_stats(ch);
		GloryConst::show_stats(ch);
		Parcel::show_stats(ch);
		send_to_char(ch, "  Сообщений на почте: %zu\r\n", mail::get_msg_count());
		send_to_char(ch, "  Передвижения: %d\r\n", motion);
		send_to_char(ch, "  Потрачено кун в магазинах2 за ребут: %d\r\n", ShopExt::get_spent_today());
		mob_stat::show_stats(ch);
		break;
	}
	case 5:
	{
		int k = 0;
		strcpy(buf, "Пустых выходов\r\n" "--------------\r\n");
		for (i = FIRST_ROOM; i <= top_of_world; i++)
		{
			for (j = 0; j < NUM_OF_DIRS; j++)
			{
				if (world[i]->dir_option[j]
					&& world[i]->dir_option[j]->to_room() == 0)
				{
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k,
						GET_ROOM_VNUM(i), world[i]->name);
				}
			}
		}
		page_string(ch->desc, buf, TRUE);
	}
	break;

	case 6:
		strcpy(buf, "Смертельных выходов\r\n" "-------------------\r\n");
		for (i = FIRST_ROOM, j = 0; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ROOM_DEATH))
				sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i]->name);
		page_string(ch->desc, buf, TRUE);
		break;
	case 7:
		strcpy(buf, "Комнаты для богов\r\n" "-----------------\r\n");
		for (i = FIRST_ROOM, j = 0; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ROOM_GODROOM))
				sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i]->name);
		page_string(ch->desc, buf, TRUE);
		break;
	case 8:
		*buf = '\0';
		send_to_char("Система негласного контроля:\r\n", ch);
		send_to_char("----------------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->snooping
				&& d->character
				&& STATE(d) == CON_PLAYING
				&& IN_ROOM(d->character) != NOWHERE
				&& ((CAN_SEE(ch, d->character) && GET_LEVEL(ch) >= GET_LEVEL(d->character))
					|| PRF_FLAGGED(ch, PRF_CODERINFO)))
			{
				sprintf(buf + strlen(buf), "%-10s - подслушивается %s (map %s).\r\n",
					GET_NAME(d->snooping->character), GET_PAD(d->character, 4), d->snoop_with_map ? "on" : "off");
			}
		}
		send_to_char(*buf ? buf : "Никто не подслушивается.\r\n", ch);
		break;		// snoop
	case 9:		// show linkdrop
		send_to_char("  Список игроков в состоянии 'link drop'\r\n", ch);
		sprintf(buf, "%-50s%-16s   %s\r\n", "   Имя", "Комната", "Бездействие (тики)");
		send_to_char(buf, ch);
		i = 0;
		for (const auto vict : character_list)
		{
			if (IS_GOD(vict) || IS_NPC(vict) || vict->desc != NULL || IN_ROOM(vict) == NOWHERE)
			{
				continue;
			}
			++i;
			sprintf(buf, "%-50s[%6d][%6d]   %d\r\n",
				vict->noclan_title().c_str(), GET_ROOM_VNUM(IN_ROOM(vict)),
				GET_ROOM_VNUM(vict->get_was_in_room()), vict->char_specials.timer);
			send_to_char(buf, ch);
		}
		sprintf(buf, "Всего - %d\r\n", i);
		send_to_char(buf, ch);
		break;
	case 10:		// show punishment
		send_to_char("  Список наказанных игроков.\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->snooping != NULL && d->character != NULL)
				continue;
			if (STATE(d) != CON_PLAYING || (GET_LEVEL(ch) < GET_LEVEL(d->character) && !PRF_FLAGGED(ch, PRF_CODERINFO)))
				continue;
			if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
				continue;
			buf[0] = 0;
			if (PLR_FLAGGED(d->character, PLR_FROZEN)
				&& FREEZE_DURATION(d->character))
				sprintf(buf + strlen(buf), "Заморожен : %ld час [%s].\r\n",
					static_cast<long>((FREEZE_DURATION(d->character) - time(NULL)) / 3600),
					FREEZE_REASON(d->character) ? FREEZE_REASON(d->character) : "-");

			if (PLR_FLAGGED(d->character, PLR_MUTE)
				&& MUTE_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет молчать : %ld час [%s].\r\n",
					static_cast<long>((MUTE_DURATION(d->character) - time(NULL)) / 3600),
					MUTE_REASON(d->character) ? MUTE_REASON(d->character) : "-");

			if (PLR_FLAGGED(d->character, PLR_DUMB)
				&& DUMB_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет нем : %ld час [%s].\r\n",
					static_cast<long>((DUMB_DURATION(d->character) - time(NULL)) / 3600),
					DUMB_REASON(d->character) ? DUMB_REASON(d->character) : "-");

			if (PLR_FLAGGED(d->character, PLR_HELLED)
				&& HELL_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет в аду : %ld час [%s].\r\n",
					static_cast<long>((HELL_DURATION(d->character) - time(NULL)) / 3600),
					HELL_REASON(d->character) ? HELL_REASON(d->character) : "-");

			if (!PLR_FLAGGED(d->character, PLR_REGISTERED)
				&& UNREG_DURATION(d->character))
			{
				sprintf(buf + strlen(buf), "Не сможет заходить с одного IP : %ld час [%s].\r\n",
					static_cast<long>((UNREG_DURATION(d->character) - time(NULL)) / 3600),
					UNREG_REASON(d->character) ? UNREG_REASON(d->character) : "-");
			}

			if (buf[0])
			{
				send_to_char(GET_NAME(d->character), ch);
				send_to_char("\r\n", ch);
				send_to_char(buf, ch);
			}
		}
		break;
	case 11:		// show paths
		if (self)
		{
			std::string out = print_zone_exits(world[ch->in_room]->zone);
			out += print_zone_enters(world[ch->in_room]->zone);
			page_string(ch->desc, out);
		}
		else if (*value && is_number(value))
		{
			for (zvn = atoi(value), zrn = 0;
				zone_table[zrn].number != zvn && zrn < static_cast<zone_rnum>(zone_table.size());
				zrn++)
			{
				// empty
			}

			if (zrn < static_cast<zone_rnum>(zone_table.size()))
			{
				auto out = print_zone_exits(zrn);
				out += print_zone_enters(zrn);
				page_string(ch->desc, out);
			}
			else
			{
				send_to_char("Нет такой зоны.\r\n", ch);
				return;
			}
		}
		else
		{
			send_to_char("Какую зону показать?\r\n", ch);
			return;
		}
		break;

	case 12:		// show loadrooms
		break;

	case 13:		// show skills
		if (!*value)
		{
			send_to_char("Уточните имя.\r\n", ch);
			return;
		}
		if (!(vict = get_player_vis(ch, value, FIND_CHAR_WORLD)))
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
		list_skills(vict, ch);
		break;
	case 14:		// show spells
		if (!*value)
		{
			send_to_char("Уточните имя.\r\n", ch);
			return;
		}
		if (!(vict = get_player_vis(ch, value, FIND_CHAR_WORLD)))
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
		list_spells(vict, ch, FALSE);
		break;
	case 15:		//Show ban. Далим.
		if (!*value)
		{
			ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
			return;
		}
		ban->ShowBannedIpByMask(BanList::SORT_BY_DATE, ch, value);
		break;
	case 16:		// show features
		if (!*value)
		{
			send_to_char("Уточните имя.\r\n", ch);
			return;
		}
		if (!(vict = get_player_vis(ch, value, FIND_CHAR_WORLD)))
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
		list_feats(vict, ch, FALSE);
		break;
	case 17:		// show glory
		GloryMisc::show_log(ch, value);
		break;
	case 18:		// show crc
		FileCRC::show(ch);
		break;
	case 19:		// show affected rooms
		RoomSpells::ShowRooms(ch);
		break;
	case 20: // money
		MoneyDropStat::print(ch);
		break;
	case 21: // expgain
		ZoneExpStat::print_gain(ch);
		break;
	case 22: // runes
		print_rune_stats(ch);
		break;
	case 23: // mobstat
	{
		if (*value && is_number(value))
		{
			if (*value1 && is_number(value1))
			{
				mob_stat::show_zone(ch, atoi(value), atoi(value1));
			}
			else
			{
				mob_stat::show_zone(ch, atoi(value), 0);
			}
		}
		else
		{
			send_to_char("Формат комнады: show mobstat внум-зоны <месяцев>.\r\n", ch);
		}
		break;
	}
	case 24: // bosses
		if (*value && !strcmp(value, "-l"))
		{
			print_mob_bosses(ch, true);
		}
		else
		{
			print_mob_bosses(ch, false);
		}
		break;
	case 25: // remort
		Remort::show_config(ch);
		break;
	default:
		send_to_char("Извините, неверная команда.\r\n", ch);
		break;
	}
}


// **************** The do_set function

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

inline void SET_OR_REMOVE(const bool on, const bool off, FLAG_DATA& flagset, const uint32_t packed_flag)
{
	if (on)
	{
		flagset.set(packed_flag);
	}
	else if (off)
	{
		flagset.unset(packed_flag);
	}
}

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


// The set options available
struct set_struct		/*
				   { const char *cmd;
				   const char level;
				   const char pcnpc;
				   const char type;
				   } */ set_fields[] =
{
	{"brief", LVL_GOD, PC, BINARY},	// 0
	{"invstart", LVL_GOD, PC, BINARY},	// 1
	{"nosummon", LVL_GRGOD, PC, BINARY},
	{"maxhit", LVL_IMPL, BOTH, NUMBER},
	{"maxmana", LVL_GRGOD, BOTH, NUMBER},
	{"maxmove", LVL_IMPL, BOTH, NUMBER},	// 5
	{"hit", LVL_GRGOD, BOTH, NUMBER},
	{"mana", LVL_GRGOD, BOTH, NUMBER},
	{"move", LVL_GRGOD, BOTH, NUMBER},
	{"race", LVL_GRGOD, BOTH, NUMBER},
	{"size", LVL_IMPL, BOTH, NUMBER},	// 10
	{"ac", LVL_GRGOD, BOTH, NUMBER},
	{"gold", LVL_IMPL, BOTH, NUMBER},
	{"bank", LVL_IMPL, PC, NUMBER},
	{"exp", LVL_IMPL, BOTH, NUMBER},
	{"hitroll", LVL_IMPL, BOTH, NUMBER}, // 15
	{"damroll", LVL_IMPL, BOTH, NUMBER},
	{"invis", LVL_IMPL, PC, NUMBER},
	{"nohassle", LVL_IMPL, PC, BINARY},
	{"frozen", LVL_GRGOD, PC, MISC},
	{"practices", LVL_GRGOD, PC, NUMBER}, // 20
	{"lessons", LVL_GRGOD, PC, NUMBER},
	{"drunk", LVL_GRGOD, BOTH, MISC},
	{"hunger", LVL_GRGOD, BOTH, MISC},
	{"thirst", LVL_GRGOD, BOTH, MISC},
	{"thief", LVL_GOD, PC, BINARY}, // 25
	{"level", LVL_IMPL, BOTH, NUMBER},
	{"room", LVL_IMPL, BOTH, NUMBER},
	{"roomflag", LVL_GRGOD, PC, BINARY},
	{"siteok", LVL_GRGOD, PC, BINARY},
	{"deleted", LVL_IMPL, PC, BINARY}, // 30
	{"class", LVL_IMPL, BOTH, MISC},
	{"demigod", LVL_IMPL, PC, BINARY},
	{"loadroom", LVL_GRGOD, PC, MISC},
	{"color", LVL_GOD, PC, BINARY},
	{"idnum", LVL_IMPL, PC, NUMBER}, // 35
	{"passwd", LVL_IMPL, PC, MISC},
	{"nodelete", LVL_GOD, PC, BINARY},
	{"sex", LVL_GRGOD, BOTH, MISC},
	{"age", LVL_GRGOD, BOTH, NUMBER},
	{"height", LVL_GOD, BOTH, NUMBER}, // 40
	{"weight", LVL_GOD, BOTH, NUMBER},
	{"godslike", LVL_IMPL, BOTH, BINARY},
	{"godscurse", LVL_IMPL, BOTH, BINARY},
	{"olc", LVL_IMPL, PC, NUMBER},
	{"name", LVL_GRGOD, PC, MISC}, // 45
	{"trgquest", LVL_IMPL, PC, MISC},
	{"mkill", LVL_IMPL, PC, MISC},
	{"highgod", LVL_IMPL, PC, MISC},
	{"hell", LVL_GOD, PC, MISC},
	{"email", LVL_GOD, PC, MISC}, //50
	{"religion", LVL_GOD, PC, MISC},
	{"perslog", LVL_IMPL, PC, BINARY},
	{"mute", LVL_GOD, PC, MISC},
	{"dumb", LVL_GOD, PC, MISC},
	{"karma", LVL_IMPL, PC, MISC},
	{"unreg", LVL_GOD, PC, MISC}, // 56
	{"executor", LVL_IMPL, PC, BINARY}, // 57
	{"killer", LVL_IMPL, PC, BINARY}, // 58
	{"remort", LVL_IMPL, PC, BINARY}, // 59
	{"tester", LVL_IMPL, PC, BINARY}, // 60
	{"autobot",LVL_IMPL, PC, BINARY}, // 61
	{"hryvn",LVL_IMPL, PC, NUMBER}, // 62
	{"\n", 0, BOTH, MISC}
};

int perform_set(CHAR_DATA * ch, CHAR_DATA * vict, int mode, char *val_arg)
{
	int i, j, c, value = 0, return_code = 1, ptnum, times = 0;
	bool on = false;
	bool off = false;
	char npad[CObjectPrototype::NUM_PADS][256];
	char *reason;
	room_rnum rnum;
	room_vnum rvnum;
	char output[MAX_STRING_LENGTH], num[MAX_INPUT_LENGTH];
	int rod;

	// Check to make sure all the levels are correct
	if (!IS_IMPL(ch))
	{
		if (!IS_NPC(vict) && vict != ch)
		{
			if (!GET_GOD_FLAG(ch, GF_DEMIGOD))
			{
				if (GET_LEVEL(ch) <= GET_LEVEL(vict) && !PRF_FLAGGED(ch, PRF_CODERINFO))
				{
					send_to_char("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			}
			else
			{
				if (GET_LEVEL(vict) >= LVL_IMMORT || PRF_FLAGGED(vict, PRF_CODERINFO))
				{
					send_to_char("Это не так просто, как вам кажется...\r\n", ch);
					return (0);
				}
			}
		}
	}
	if (!Privilege::can_do_priv(ch, std::string(set_fields[mode].cmd), 0, 1))
	{
		send_to_char("Кем вы себя возомнили?\r\n", ch);
		return (0);
	}

	// Make sure the PC/NPC is correct
	if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
	{
		send_to_char("Эта тварь недостойна такой чести!\r\n", ch);
		return (0);
	}
	else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC))
	{
		act("Вы оскорбляете $S - $E ведь не моб!", FALSE, ch, 0, vict, TO_CHAR);
		return (0);
	}

	// Find the value of the argument
	if (set_fields[mode].type == BINARY)
	{
		if (!strn_cmp(val_arg, "on", 2) || !strn_cmp(val_arg, "yes", 3) || !strn_cmp(val_arg, "вкл", 3))
			on = 1;
		else if (!strn_cmp(val_arg, "off", 3) || !strn_cmp(val_arg, "no", 2) || !strn_cmp(val_arg, "выкл", 4))
			off = 1;
		if (!(on || off))
		{
			send_to_char("Значение может быть 'on' или 'off'.\r\n", ch);
			return (0);
		}
		sprintf(output, "%s %s для %s.", set_fields[mode].cmd, ONOFF(on), GET_PAD(vict, 1));
	}
	else if (set_fields[mode].type == NUMBER)
	{
		value = atoi(val_arg);
		sprintf(output, "У %s %s установлено в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
	}
	else
	{
		strcpy(output, "Хорошо.");
	}
	switch (mode)
	{
	case 0:
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_BRIEF);
		break;
	case 1:
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_INVSTART);
		break;
	case 2:
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_SUMMONABLE);
		sprintf(output, "Возможность призыва %s для %s.\r\n", ONOFF(!on), GET_PAD(vict, 1));
		break;
	case 3:
		vict->points.max_hit = RANGE(1, 5000);
		affect_total(vict);
		break;
	case 4:
		break;
	case 5:
		vict->points.max_move = RANGE(1, 5000);
		affect_total(vict);
		break;
	case 6:
		vict->points.hit = RANGE(-9, vict->points.max_hit);
		affect_total(vict);
		break;
	case 7:
		break;
	case 8:
		break;
	case 9:
		// Выставляется род для РС
		rod = PlayerRace::CheckRace(GET_KIN(ch), val_arg);
		if (rod == RACE_UNDEFINED)
		{
			send_to_char("Не было таких на земле русской!\r\n", ch);
			send_to_char(PlayerRace::ShowRacesMenu(GET_KIN(ch)), ch);
			return (0);
		}
		else
		{
			GET_RACE(vict) = rod;
			affect_total(vict);

		}
		break;
	case 10:
		vict->real_abils.size = RANGE(1, 100);
		affect_total(vict);
		break;
	case 11:
		vict->real_abils.armor = RANGE(-100, 100);
		affect_total(vict);
		break;
	case 12:
		vict->set_gold(value);
		break;
	case 13:
		vict->set_bank(value);
		break;
	case 14:
		//vict->points.exp = RANGE(0, 7000000);
		RANGE(0, level_exp(vict, LVL_IMMORT) - 1);
		gain_exp_regardless(vict, value - GET_EXP(vict));
		break;
	case 15:
		vict->real_abils.hitroll = RANGE(-20, 20);
		affect_total(vict);
		break;
	case 16:
		vict->real_abils.damroll = RANGE(-20, 20);
		affect_total(vict);
		break;
	case 17:
		if (!IS_IMPL(ch) && ch != vict && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return (0);
		}
		SET_INVIS_LEV(vict, RANGE(0, GET_LEVEL(vict)));
		break;
	case 18:
		if (!IS_IMPL(ch) && ch != vict && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return (0);
		}
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_NOHASSLE);
		break;
	case 19:
		reason = one_argument(val_arg, num);
		if (*num) times = atol(num);
		if (!set_punish(ch, vict, SCMD_FREEZE, reason, times)) return (0);
		break;
	case 20:
	case 21:
		return_code = 0;
		break;
	case 22:
	case 23:
	case 24:
	{
		const unsigned num = mode - 22; // magic number циркулевских времен
		if (num >= (ch)->player_specials->saved.conditions.size())
		{
			send_to_char("Ошибка: num >= saved.conditions.size(), сообщите кодерам.\r\n", ch);
			return 0;
		}
		if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл"))
		{
			GET_COND(vict, num) = -1;
			sprintf(output, "Для %s %s сейчас отключен.", GET_PAD(vict, 1), set_fields[mode].cmd);
		}
		else if (is_number(val_arg))
		{
			value = atoi(val_arg);
			RANGE(0, MAX_COND_VALUE);
			GET_COND(vict, num) = value;
			sprintf(output, "Для %s %s установлен в %d.", GET_PAD(vict, 1), set_fields[mode].cmd, value);
		}
		else
		{
			send_to_char("Должно быть 'off' или значение от 0 до 24.\r\n", ch);
			return 0;
		}
		break;
	}
	case 25:
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_THIEF);
		break;
	case 26:
		if (!PRF_FLAGGED(ch, PRF_CODERINFO)
			&& (value > GET_LEVEL(ch) || value > LVL_IMPL || GET_LEVEL(vict) > GET_LEVEL(ch)))
		{
			send_to_char("Вы не можете установить уровень игрока выше собственного.\r\n", ch);
			return (0);
		}
		RANGE(0, LVL_IMPL);
		vict->set_level(value);
		break;
	case 27:
		if ((rnum = real_room(value)) == NOWHERE)
		{
			send_to_char("Поищите другой МУД. В этом МУДе нет такой комнаты.\r\n", ch);
			return (0);
		}
		if (IN_ROOM(vict) != NOWHERE)	// Another Eric Green special.
			char_from_room(vict);
		char_to_room(vict, rnum);
		check_horse(vict);
		break;
	case 28:
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_ROOMFLAGS);
		break;
	case 29:
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_SITEOK);
		break;
	case 30:
		if (IS_IMPL(vict) || PRF_FLAGGED(vict, PRF_CODERINFO))
		{
			send_to_char("Истинные боги вечны!\r\n", ch);
			return 0;
		}
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_DELETED);
		break;
	case 31:
		if ((i = parse_class(*val_arg)) == CLASS_UNDEFINED)
		{
			send_to_char("Нет такого класса в этой игре. Найдите себе другую.\r\n", ch);
			return (0);
		}
		vict->set_class(i);
		break;
	case 32:
		// Флаг для морталов с привилегиями
		if (!IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return 0;
		}
		if (on)
		{
			SET_GOD_FLAG(vict, GF_DEMIGOD);
		}
		else if (off)
		{
			CLR_GOD_FLAG(vict, GF_DEMIGOD);
		}
		break;
	case 33:
		if (is_number(val_arg))
		{
			rvnum = atoi(val_arg);
			if (real_room(rvnum) != NOWHERE)
			{
				GET_LOADROOM(vict) = rvnum;
				sprintf(output, "%s будет входить в игру из комнаты #%d.",
					GET_NAME(vict), GET_LOADROOM(vict));
			}
			else
			{
				send_to_char
				("Прежде чем кого-то куда-то поместить, надо это КУДА-ТО создать.\r\n"
					"Скажите Стрибогу - пусть зоны рисует, а не пьянствует.\r\n", ch);
				return (0);
			}
		}
		else
		{
			send_to_char("Должен быть виртуальный номер комнаты.\r\n", ch);
			return (0);
		}
		break;
	case 34:
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_COLOR_1);
		SET_OR_REMOVE(on, off, PRF_FLAGS(vict), PRF_COLOR_2);
		break;
	case 35:
		if (!IS_IMPL(ch) || !IS_NPC(vict))
		{
			return (0);
		}
		vict->set_idnum(value);
		break;
	case 36:
		if (!IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO) && ch != vict)
		{
			send_to_char("Давайте не будем экспериментировать.\r\n", ch);
			return (0);
		}
		if (IS_IMPL(vict) && ch != vict)
		{
			send_to_char("Вы не можете ЭТО изменить.\r\n", ch);
			return (0);
		}
		if (!Password::check_password(vict, val_arg))
		{
			send_to_char(ch, "%s\r\n", Password::BAD_PASSWORD);
			return 0;
		}
		Password::set_password(vict, val_arg);
		Password::send_password(GET_EMAIL(vict), val_arg, std::string(GET_NAME(vict)));
		sprintf(buf, "%s заменен пароль богом.", GET_PAD(vict, 2));
		add_karma(vict, buf, GET_NAME(ch));
		sprintf(output, "Пароль изменен на '%s'.", val_arg);
		break;
	case 37:
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_NODELETE);
		break;
	case 38:
		if ((i = search_block(val_arg, genders, FALSE)) < 0)
		{
			send_to_char
			("Может быть 'мужчина', 'женщина', или 'бесполое'(а вот это я еще не оценил :).\r\n", ch);
			return (0);
		}
		GET_SEX(vict) = static_cast<ESex>(i);
		break;

	case 39:		// set age
		if (value < 2 || value > 200)  	// Arbitrary limits.
		{
			send_to_char("Поддерживаются возрасты от 2 до 200.\r\n", ch);
			return (0);
		}
		/*
		 * NOTE: May not display the exact age specified due to the integer
		 * division used elsewhere in the code.  Seems to only happen for
		 * some values below the starting age (17) anyway. -gg 5/27/98
		 */
		vict->player_data.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
		break;

	case 40:		// Blame/Thank Rick Glover. :)
		GET_HEIGHT(vict) = value;
		affect_total(vict);
		break;

	case 41:
		GET_WEIGHT(vict) = value;
		affect_total(vict);
		break;

	case 42:
		if (on)
		{
			SET_GOD_FLAG(vict, GF_GODSLIKE);
			if (sscanf(val_arg, "%s %d", npad[0], &i) != 0)
				GCURSE_DURATION(vict) = (i > 0) ? time(NULL) + i * 60 * 60 : MAX_TIME;
			else
				GCURSE_DURATION(vict) = 0;
			sprintf(buf, "%s установил GUDSLIKE персонажу %s.", GET_NAME(ch), GET_NAME(vict));
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, 0);

		}
		else if (off)
			CLR_GOD_FLAG(vict, GF_GODSLIKE);
		break;
	case 43:
		if (on)
		{
			SET_GOD_FLAG(vict, GF_GODSCURSE);
			if (sscanf(val_arg, "%s %d", npad[0], &i) != 0)
				GCURSE_DURATION(vict) = (i > 0) ? time(NULL) + i * 60 * 60 : MAX_TIME;
			else
				GCURSE_DURATION(vict) = 0;
		}
		else if (off)
			CLR_GOD_FLAG(vict, GF_GODSCURSE);
		break;
	case 44:
		if (PRF_FLAGGED(ch, PRF_CODERINFO) || IS_IMPL(ch))
			GET_OLC_ZONE(vict) = value;
		else
		{
			sprintf(buf, "Слишком низкий уровень чтоб раздавать права OLC.\r\n");
			send_to_char(buf, ch);
		}
		break;

	case 45:
		// изменение имени !!!

		if ((i = sscanf(val_arg, "%s %s %s %s %s %s", npad[0], npad[1], npad[2], npad[3], npad[4], npad[5])) != 6)
		{
			sprintf(buf, "Требуется указать 6 падежей, найдено %d\r\n", i);
			send_to_char(buf, ch);
			return (0);
		}

		if (*npad[0] == '*')  	// Only change pads
		{
			for (i = 1; i < CObjectPrototype::NUM_PADS; i++)
				if (!_parse_name(npad[i], npad[i]))
				{
					vict->player_data.PNames[i] = std::string(npad[i]);
				}
			sprintf(buf, "Произведена замена падежей.\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			for (i = 0; i < CObjectPrototype::NUM_PADS; i++)
			{
				if (strlen(npad[i]) < MIN_NAME_LENGTH || strlen(npad[i]) > MAX_NAME_LENGTH)
				{
					sprintf(buf, "Падеж номер %d некорректен.\r\n", ++i);
					send_to_char(buf, ch);
					return (0);
				}
			}

			if (_parse_name(npad[0], npad[0]) ||
				strlen(npad[0]) < MIN_NAME_LENGTH ||
				strlen(npad[0]) > MAX_NAME_LENGTH ||
				!Valid_Name(npad[0]) || reserved_word(npad[0]) || fill_word(npad[0]))
			{
				send_to_char("Некорректное имя.\r\n", ch);
				return (0);
			}

			if ((get_id_by_name(npad[0]) >= 0) && !PLR_FLAGS(vict).get(PLR_DELETED))
			{
				send_to_char("Это имя совпадает с именем другого персонажа.\r\n"
					"Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
				return (0);
			}
			// выносим из листа неодобренных имен, если есть
			NewNames::remove(vict);

			ptnum = get_ptable_by_name(GET_NAME(vict));
			if (ptnum < 0)
				return (0);

			if (!PLR_FLAGS(vict).get(PLR_FROZEN)
				&& !PLR_FLAGS(vict).get(PLR_DELETED)
				&& !IS_IMMORTAL(vict))
			{
				TopPlayer::Remove(vict);
			}

			for (i = 0; i < CObjectPrototype::NUM_PADS; i++)
			{
				if (!_parse_name(npad[i], npad[i]))
				{
					vict->player_data.PNames[i] = std::string(npad[i]);
				}
			}
			sprintf(buf, "Name changed from %s to %s", GET_NAME(vict), npad[0]);
			vict->set_name(npad[0]);
			add_karma(vict, buf, GET_NAME(ch));

			if (!PLR_FLAGS(vict).get(PLR_FROZEN)
				&& !PLR_FLAGS(vict).get(PLR_DELETED)
				&& !IS_IMMORTAL(vict))
			{
				TopPlayer::Refresh(vict);
			}

			player_table.set_name(ptnum, npad[0]);

			return_code = 2;
			PLR_FLAGS(vict).set(PLR_CRASH);
		}
		break;

	case 46:

		npad[1][0] = '\0';
		if (sscanf(val_arg, "%d %s %[^\n]", &ptnum, npad[0], npad[1]) != 3)
		{
			if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2)
			{
				send_to_char("Формат : set <имя> trgquest <quest_num> <on|off> <строка данных>\r\n", ch);
				return 0;
			}
		}

		if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл"))
		{
			if (!vict->quested_remove(ptnum))
			{
				act("$N не выполнял$G этого квеста.", FALSE, ch, 0, vict, TO_CHAR);
				return 0;
			}
		}
		else if (!str_cmp(npad[0], "on") || !str_cmp(npad[0], "вкл"))
		{
			vict->quested_add(vict, ptnum, npad[1]);
		}
		else
		{
			send_to_char("Требуется on или off.\r\n", ch);
			return 0;
		}
		break;

	case 47:

		if (sscanf(val_arg, "%d %s", &ptnum, npad[0]) != 2)
		{
			send_to_char("Формат : set <имя> mkill <mob_vnum> <off|num>\r\n", ch);
			return (0);
		}
		if (!str_cmp(npad[0], "off") || !str_cmp(npad[0], "выкл"))
			vict->mobmax_remove(ptnum);
		else if ((j = atoi(npad[0])) > 0)
		{
			if ((c = vict->mobmax_get(ptnum)) != j)
				vict->mobmax_add(vict, ptnum, j - c, MobMax::get_level_by_vnum(ptnum));
			else
			{
				act("$N убил$G именно столько этих мобов.", FALSE, ch, 0, vict, TO_CHAR);
				return (0);
			}
		}
		else
		{
			send_to_char("Требуется off или значение больше 0.\r\n", ch);
			return (0);
		}
		break;

	case 48:
		return (0);
		break;

	case 49:
		reason = one_argument(val_arg, num);
		if (*num) times = atol(num);
		if (!set_punish(ch, vict, SCMD_HELL, reason, times)) return (0);
		break;

	case 50:
		if (valid_email(val_arg))
		{
			lower_convert(val_arg);
			sprintf(buf, "Email changed from %s to %s", GET_EMAIL(vict), val_arg);
			add_karma(vict, buf, GET_NAME(ch));
			strncpy(GET_EMAIL(vict), val_arg, 127);
			*(GET_EMAIL(vict) + 127) = '\0';
		}
		else
		{
			send_to_char("Wrong E-Mail.\r\n", ch);
			return (0);
		}
		break;

	case 51:
		// Выставляется род для РС
		rod = (*val_arg);
		if (rod != '0' && rod != '1')
		{
			send_to_char("Не было таких на земле русской!\r\n", ch);
			send_to_char("0 - Язычество, 1 - Христианство\r\n", ch);
			return (0);
		}
		else
		{
			GET_RELIGION(vict) = rod - '0';
		}
		break;

	case 52:
		// Отдельный лог команд персонажа
		if (on)
		{
			SET_GOD_FLAG(vict, GF_PERSLOG);
		}
		else if (off)
		{
			CLR_GOD_FLAG(vict, GF_PERSLOG);
		}
		break;

	case 53:
		reason = one_argument(val_arg, num);
		if (*num) times = atol(num);
		if (!set_punish(ch, vict, SCMD_MUTE, reason, times)) return (0);
		break;

	case 54:
		reason = one_argument(val_arg, num);
		if (*num) times = atol(num);
		if (!set_punish(ch, vict, SCMD_DUMB, reason, times)) return (0);
		break;

	case 55:
		if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Кем вы себя возомнили?\r\n", ch);
			return 0;
		}
		reason = one_argument(val_arg, num);
		if (*num && reason && *reason)
		{
			skip_spaces(&reason);
			sprintf(buf, "%s by %s", num, GET_NAME(ch));
			if (!strcmp(reason, "clear"))
			{
				if KARMA(vict)
					free(KARMA(vict));

				KARMA(vict) = 0;
				act("Вы отпустили $N2 все грехи.", FALSE, ch, 0, vict, TO_CHAR);
				sprintf(buf, "%s", GET_NAME(ch));
				add_karma(vict, "Очистка грехов", buf);

			}
			else  add_karma(vict, buf, reason);
		}
		else
		{
			send_to_char("Формат команды: set [ file | player ] <character> karma <action> <reason>\r\n", ch);
			return (0);
		}
		break;

	case 56:      // Разрегистрация персонажа
		reason = one_argument(val_arg, num);
		if (*num) times = atol(num);
		if (!set_punish(ch, vict, SCMD_REGISTER, reason, times)) return (0);
		break;

	case 57:      // Установка флага палач
		reason = one_argument(val_arg, num);
		skip_spaces(&reason);
		sprintf(buf, "executor %s by %s", (on ? "on" : "off"), GET_NAME(ch));
		add_karma(vict, buf, reason);
		if (on)
		{
			PRF_FLAGS(vict).set(PRF_EXECUTOR);
		}
		else if (off)
		{
			PRF_FLAGS(vict).unset(PRF_EXECUTOR);
		}
		break;

	case 58: // Снятие или постановка флага !ДУШЕГУБ! только для имплементоров
		SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_KILLER);
		break;
	case 59: // флаг реморта
		ch->remort();
		sprintf(buf, "Иммортал %s установил реморт +1 для игрока %s", GET_NAME(ch), GET_NAME(vict));
		add_karma(vict, buf, GET_NAME(ch));
		add_karma(ch, buf, GET_NAME(vict));
		send_to_gods(buf);
		break;
	case 60: // флаг тестера
		if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "выкл"))
		{
			CLR_GOD_FLAG(vict, GF_TESTER);
			PRF_FLAGS(vict).unset(PRF_TESTER); // обнулим реж тестер
			sprintf(buf, "%s убрал флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
		}
		else
		{
			SET_GOD_FLAG(vict, GF_TESTER);
			sprintf(buf, "%s установил флаг тестера для игрока %s", GET_NAME(ch), GET_NAME(vict));
			mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
			//			send_to_gods(buf);
		}
		break;
	case 61: // флаг автобота
		{
			SET_OR_REMOVE(on, off, PLR_FLAGS(vict), PLR_AUTOBOT);
			break;
		}
	case 62:
		vict->set_hryvn(value);
		break;

	default:
		send_to_char("Не могу установить это!\r\n", ch);
		return (0);
	}

	strcat(output, "\r\n");
	send_to_char(CAP(output), ch);
	return (return_code);
}

void do_set(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict = NULL;
	char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH], OName[MAX_INPUT_LENGTH];
	int mode, player_i = 0, retval;
	char is_file = 0, is_player = 0;

	half_chop(argument, name, buf);

	if (!*name)
	{
		strcpy(buf, "Возможные поля для изменения:\r\n");
		for (int i = 0; set_fields[i].level; i++)
			if (Privilege::can_do_priv(ch, std::string(set_fields[i].cmd), 0, 1))
				sprintf(buf + strlen(buf), "%-15s%s", set_fields[i].cmd, (!((i + 1) % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		return;
	}

	if (!strcmp(name, "file"))
	{
		is_file = 1;
		half_chop(buf, name, buf);
	}
	else if (!str_cmp(name, "player"))
	{
		is_player = 1;
		half_chop(buf, name, buf);
	}
	else if (!str_cmp(name, "mob"))
	{
		half_chop(buf, name, buf);
	}
	else
		is_player = 1;

	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*name || !*field)
	{
		send_to_char("Usage: set [mob|player|file] <victim> <field> <value>\r\n", ch);
		return;
	}

	CHAR_DATA::shared_ptr cbuf;
	// find the target
	if (!is_file)
	{
		if (is_player)
		{

			if (!(vict = get_player_pun(ch, name, FIND_CHAR_WORLD)))
			{
				send_to_char("Нет такого игрока.\r\n", ch);
				return;
			}

			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, GF_DEMIGOD))
			{
				if ((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !(is_head(ch->get_name_str())))
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
			else
			{
				if (GET_LEVEL(vict) >= LVL_IMMORT)
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
		}
		else  	// is_mob
		{
			if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))
				|| !IS_NPC(vict))
			{
				send_to_char("Нет такой твари Божьей.\r\n", ch);
				return;
			}
		}
	}
	else if (is_file)  	// try to load the player off disk
	{
		if (get_player_pun(ch, name, FIND_CHAR_WORLD))
		{
			send_to_char("Да разуй же глаза! Оно в сети!\r\n", ch);
			return;
		}

		cbuf = std::make_unique<Player>();
		if ((player_i = load_char(name, cbuf.get())) > -1)
		{
			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, GF_DEMIGOD))
			{
				if (GET_LEVEL(ch) <= GET_LEVEL(cbuf) && !(is_head(ch->get_name_str())))
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
			else
			{
				if (GET_LEVEL(cbuf) >= LVL_IMMORT)
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					return;
				}
			}
			vict = cbuf.get();
		}
		else
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
	}

	// find the command in the list
	size_t len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
	{
		if (!strncmp(field, set_fields[mode].cmd, len))
		{
			break;
		}
	}

	// perform the set
	strcpy(OName, GET_NAME(vict));
	retval = perform_set(ch, vict, mode, val_arg);

	// save the character if a change was made
	if (retval && !IS_NPC(vict))
	{
		if (retval == 2)
		{
			rename_char(vict, OName);
		}
		else
		{
			if (!is_file && !IS_NPC(vict))
			{
				vict->save_char();
			}
			if (is_file)
			{
				vict->save_char();
				send_to_char("Файл сохранен.\r\n", ch);
			}
		}
	}

	log("(GC) %s try to set: %s", GET_NAME(ch), argument);
	imm_log("%s try to set: %s", GET_NAME(ch), argument);
}

int shop_ext(CHAR_DATA *ch, void *me, int cmd, char* argument);
int receptionist(CHAR_DATA *ch, void *me, int cmd, char* argument);
int postmaster(CHAR_DATA *ch, void *me, int cmd, char* argument);
int bank(CHAR_DATA *ch, void *me, int cmd, char* argument);
int exchange(CHAR_DATA *ch, void *me, int cmd, char* argument);
int horse_keeper(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_mono(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_poly(CHAR_DATA *ch, void *me, int cmd, char* argument);

namespace Mlist
{

	std::string print_race(CHAR_DATA *mob)
	{
		std::string out;
		if (GET_RACE(mob) < NPC_RACE_NEXT)
		{
			out += npc_race_types[GET_RACE(mob) - NPC_RACE_BASIC];
		}
		else
		{
			out += "UNDEF";
		}
		return out;
	}

	std::string print_role(CHAR_DATA *mob)
	{
		std::string out;
		if (mob->get_role_bits().any())
		{
			print_bitset(mob->get_role_bits(), npc_role_types, ",", out);
		}
		else
		{
			out += "---";
		}
		return out;
	}

	std::string print_script(CHAR_DATA *mob, const std::string &key)
	{
		std::string out;

		bool print_name = false;
		if (key == "scriptname" || key == "triggername")
		{
			print_name = true;
		}

		if (!mob_proto[GET_MOB_RNUM(mob)].proto_script->empty())
		{
			bool first = true;
			for (const auto trigger_vnum : *mob_proto[GET_MOB_RNUM(mob)].proto_script)
			{
				const int trg_rnum = real_trigger(trigger_vnum);
				if (trg_rnum >= 0)
				{
					if (!first)
					{
						out += ", ";
					}
					else
					{
						first = false;
					}
					out += boost::lexical_cast<std::string>(trig_index[trg_rnum]->vnum);
					if (print_name)
					{
						out += "(";
						const auto& trigger_name = trig_index[trg_rnum]->proto->get_name();
						out += !trigger_name.empty() ? trigger_name.c_str() : "null";
						out += ")";
					}
				}
			}
		}
		else
		{
			out += "---";
		}

		return out;
	}

	std::string print_special(CHAR_DATA *mob)
	{
		std::string out;

		if (mob_index[GET_MOB_RNUM(mob)].func)
		{
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
			else if (func == guild_mono)
				out += "teacher (mono)";
			else if (func == guild_poly)
				out += "teacher (poly)";
			else if (func == torc)
				out += "torc";
			else if (func == Noob::outfit)
				out += "outfit";
		}
		else
		{
			out += "---";
		}

		return out;
	}


	std::string print_flag(CHAR_DATA *ch, CHAR_DATA *mob, const std::string &options)
	{
		std::vector<std::string> option_list;
		boost::split(option_list, options, boost::is_any_of(", "), boost::token_compress_on);

		std::string out;
		for (std::vector<std::string>::const_iterator i = option_list.begin(),
			iend = option_list.end(); i != iend; ++i)
		{
			if (isname(*i, "race"))
			{
				out += boost::str(boost::format(" [раса: %s%s%s ]")
					% CCCYN(ch, C_NRM) % print_race(mob) % CCNRM(ch, C_NRM));
			}
			else if (isname(*i, "role"))
			{
				out += boost::str(boost::format(" [роли: %s%s%s ]")
					% CCCYN(ch, C_NRM) % print_role(mob) % CCNRM(ch, C_NRM));
			}
			else if (isname(*i, "script trigger scriptname triggername"))
			{
				out += boost::str(boost::format(" [скрипты: %s%s%s ]")
					% CCCYN(ch, C_NRM) % print_script(mob, *i) % CCNRM(ch, C_NRM));
			}
			else if (isname(*i, "special"))
			{
				out += boost::str(boost::format(" [спец-проц: %s%s%s ]")
					% CCCYN(ch, C_NRM) % print_special(mob) % CCNRM(ch, C_NRM));
			}
		}

		return out;
	}

	void print(CHAR_DATA *ch, int first, int last, const std::string &options)
	{
		std::stringstream out;
		out << "Список мобов от " << first << " до " << last << "\r\n";
		int cnt = 0;
		for (int i = 0; i <= top_of_mobt; ++i)
		{
			if (mob_index[i].vnum >= first && mob_index[i].vnum <= last)
			{
				out << boost::format("%5d. %45s [%6d] [%2d]%s")
					% ++cnt
					% (mob_proto[i].get_name_str().size() > 45
						? mob_proto[i].get_name_str().substr(0, 45)
						: mob_proto[i].get_name_str())
					% mob_index[i].vnum
					% mob_proto[i].get_level()
					% print_flag(ch, mob_proto + i, options);
				if (!mob_proto[i].proto_script->empty())
				{
					out << " - есть скрипты -";
					for (const auto trigger_vnum : *mob_proto[i].proto_script)
					{
						sprintf(buf1, " [%d]", trigger_vnum);
						out << buf1;
					}
					out << "\r\n";
				}
				else
					out << " - нет скриптов\r\n";
			}
		}

		if (cnt == 0)
		{
			send_to_char("Нет мобов в этом промежутке.\r\n", ch);
		}
		else
		{
			page_string(ch->desc, out.str());
		}
	}

} // namespace Mlist

int print_olist(const CHAR_DATA* ch, const int first, const int last, std::string& out)
{
	int result = 0;

	char buf_[256] = { 0 };
	std::stringstream ss;
	snprintf(buf_, sizeof(buf_), "Список объектов Vnum %d до %d\r\n", first, last);
	ss << buf_;

	auto from = obj_proto.vnum2index().lower_bound(first);
	auto to = obj_proto.vnum2index().upper_bound(last);
	for (auto i = from; i != to; ++i)
	{
		const auto vnum = i->first;
		const auto rnum = i->second;
		const auto prototype = obj_proto[rnum];
		snprintf(buf_, sizeof(buf_), "%5d. %s [%5d] [ilvl=%f : mort =%d]", ++result,
			colored_name(prototype->get_short_description().c_str(), 45),
			vnum, prototype->get_ilevel(), prototype->get_auto_mort_req());
		ss << buf_;

		if (GET_LEVEL(ch) >= LVL_GRGOD
			|| PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			snprintf(buf_, sizeof(buf_), " Игра:%d Пост:%d Макс:%d",
				obj_proto.number(rnum),
				obj_proto.stored(rnum), GET_OBJ_MIW(obj_proto[rnum]));
			ss << buf_;

			const auto& script = prototype->get_proto_script();
			if (!script.empty())
			{
				ss << " - есть скрипты -";
				for (const auto trigger_vnum : script)
				{
					sprintf(buf1, " [%d]", trigger_vnum);
					ss << buf1;
				}
			}
			else
			{
				ss << " - нет скриптов";
			}
		}

		ss << "\r\n";
	}

	out = ss.str();
	return result;
}

void do_liblist(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{

	int first, last, nr, found = 0;

	argument = two_arguments(argument, buf, buf2);

	if (!*buf || (!*buf2 && (subcmd == SCMD_ZLIST)))
	{
		switch (subcmd)
		{
		case SCMD_RLIST:
			send_to_char("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n", ch);
			break;
		case SCMD_OLIST:
			send_to_char("Использование: осписок <начальный номер или номер зоны> [<конечный номер>]\r\n", ch);
			break;
		case SCMD_MLIST:
			send_to_char("Использование: мсписок <начальный номер или номер зоны> [<конечный номер>]\r\n", ch);
			break;
		case SCMD_ZLIST:
			send_to_char("Использование: зсписок <начальный номер> <конечный номер>\r\n", ch);
			break;
		case SCMD_CLIST:
			send_to_char("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n", ch);
			break;
		default:
			sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
			mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
			break;
		}
		return;
	}
	first = atoi(buf);
	if (*buf2 && a_isdigit(buf2[0]))
	{
		last = atoi(buf2);
	}
	else
	{
		first *= 100;
		last = first + 99;
	}

	if ((first < 0) || (first > MAX_PROTO_NUMBER) || (last < 0) || (last > MAX_PROTO_NUMBER))
	{
		sprintf(buf, "Значения должны быть между 0 и %d.\n\r", MAX_PROTO_NUMBER);
		send_to_char(buf, ch);
		return;
	}

	if (first > last)
	{
		std::swap(first, last);
	}

	if (first + 200 < last)
	{
		send_to_char("Максимальный показываемый промежуток - 200.\n\r", ch);
		return;
	}

	char buf_[256];
	std::string out;

	switch (subcmd)
	{
	case SCMD_RLIST:
		snprintf(buf_, sizeof(buf_),
			"Список комнат от Vnum %d до %d\r\n", first, last);
		out += buf_;
		for (nr = FIRST_ROOM; nr <= top_of_world && (world[nr]->number <= last); nr++)
		{
			if (world[nr]->number >= first)
			{
				snprintf(buf_, sizeof(buf_), "%5d. [%5d] (%3d) %s",
					++found, world[nr]->number, world[nr]->zone, world[nr]->name);
				out += buf_;
				if (!world[nr]->proto_script->empty())
				{
					out += " - есть скрипты -";
					for (const auto trigger_vnum : *world[nr]->proto_script)
					{
						sprintf(buf1, " [%d]", trigger_vnum);
						out += buf1;
					}
					out += "\r\n";
				}
				else
				{
					out += " - нет скриптов\r\n";
				}
			}
		}
		break;
	case SCMD_OLIST:
		found = print_olist(ch, first, last, out);
		break;

	case SCMD_MLIST:
	{
		std::string option;
		if (*buf2 && !a_isdigit(buf2[0]))
		{
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

		for (nr = 0; nr < static_cast<zone_rnum>(zone_table.size()) && (zone_table[nr].number <= last); nr++)
		{
			if (zone_table[nr].number >= first)
			{
				snprintf(buf_, sizeof(buf_),
					"%5d. [%s%s] [%5d] (%3d) (%2d/%2d) (%2d) %s\r\n",
					++found,
					zone_table[nr].locked ? "L" : " ",
					zone_table[nr].under_construction ? "T" : " ",
					zone_table[nr].number,
					zone_table[nr].lifespan,
					zone_table[nr].level,
					zone_table[nr].mob_level,
					zone_table[nr].group,
					zone_table[nr].name);
				out += buf_;
			}
		}
		break;

	case SCMD_CLIST:
		out = "Заглушка. Возможно, будет использоваться в будущем\r\n";
		break;

	default:
		sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
		mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
		return;
	}

	if (!found)
	{
		switch (subcmd)
		{
		case SCMD_RLIST:
			send_to_char("Нет комнат в этом промежутке.\r\n", ch);
			break;
		case SCMD_OLIST:
			send_to_char("Нет объектов в этом промежутке.\r\n", ch);
			break;
		case SCMD_ZLIST:
			send_to_char("Нет зон в этом промежутке.\r\n", ch);
			break;
		default:
			sprintf(buf, "SYSERR:: invalid SCMD passed to do_build_list!");
			mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
			break;
		}
		return;
	}

	page_string(ch->desc, out);
}

void do_forcetime(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int m, t = 0;
	char *ca;

	// Parse command line
	for (ca = strtok(argument, " "); ca; ca = strtok(NULL, " "))
	{
		m = LOWER(ca[strlen(ca) - 1]);
		if (m == 'h')	// hours
			m = 60 * 60;
		else if (m == 'm')	// minutes
			m = 60;
		else if (m == 's' || a_isdigit(m))	// seconds
			m = 1;
		else
			m = 0;
		if ((m *= atoi(ca)) > 0)
			t += m;
		else
		{
			send_to_char("Сдвиг игрового времени (h - часы, m - минуты, s - секунды).\r\n", ch);
			return;
		}
	}

	if (!t)			// 1 tick default
	{
		t = (SECS_PER_MUD_HOUR);
	}

	for (m = 0; m < t * PASSES_PER_SEC; m++)
	{
		GlobalObjects::heartbeat()(t * PASSES_PER_SEC - m);
	}

	sprintf(buf, "(GC) %s перевел игровое время на %d сек.", GET_NAME(ch), t);
	mudlog(buf, NRM, LVL_IMMORT, IMLOG, FALSE);
	send_to_char(OK, ch);

}

///////////////////////////////////////////////////////////////////////////////
//Polud статистика использования заклинаний
namespace SpellUsage
{
	bool isActive = false;
	std::map <int, SpellCountType> usage;
	const char* SPELL_STAT_FILE = LIB_STAT"spellstat.txt";
	time_t start;
}

void SpellUsage::clear()
{
	for (std::map<int, SpellCountType>::iterator it = usage.begin(); it != usage.end(); ++it)
		it->second.clear();
	usage.clear();
	start = time(0);
}

std::string statToPrint()
{
	std::stringstream out;
	time_t now = time(0);
	char * end_time = str_dup(rustime(localtime(&now)));
	out << rustime(localtime(&SpellUsage::start)) << " - " << end_time << "\n";
	for (std::map<int, SpellCountType>::iterator it = SpellUsage::usage.begin(); it != SpellUsage::usage.end(); ++it)
	{
		out << std::setw(35) << pc_class_types[it->first] << "\n";
		for (SpellCountType::iterator itt = it->second.begin(); itt != it->second.end(); ++itt)
			out << std::setw(25) << spell_info[itt->first].name << " : " << itt->second << "\n";
	}
	return out.str();
}

void SpellUsage::save()
{
	if (!isActive)
		return;

	std::ofstream file(SPELL_STAT_FILE, std::ios_base::app | std::ios_base::out);

	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", SPELL_STAT_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	file << statToPrint();
	file.close();
}

void SpellUsage::AddSpellStat(int charClass, int spellNum)
{
	if (!isActive)
		return;
	if (charClass > NUM_PLAYER_CLASSES || spellNum > MAX_SPELLS)
		return;
	usage[charClass][spellNum]++;
}

void do_spellstat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("заклстат [стоп|старт|очистить|показать|сохранить]\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "старт"))
	{
		SpellUsage::isActive = true;
		SpellUsage::start = time(0);
		send_to_char("Сбор включен.\r\n", ch);
		return;
	}

	if (!SpellUsage::isActive)
	{
		send_to_char("Сбор выключен. Включите сбор 'заклстат старт'.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "стоп"))
	{
		SpellUsage::clear();
		SpellUsage::isActive = false;
		send_to_char("Сбор выключен.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "показать"))
	{
		page_string(ch->desc, statToPrint());
		return;
	}

	if (!str_cmp(argument, "очистить"))
	{
		SpellUsage::clear();
		return;
	}

	if (!str_cmp(argument, "сохранить"))
	{
		SpellUsage::save();
		return;
	}

	send_to_char("заклстат: неизвестный аргумент\r\n", ch);
}

void do_sanitize(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	send_to_char("Запущена процедура сбора мусора после праздника...\r\n", ch);
	Celebrates::sanitize();
}

// This is test command for different testings
void do_godtest(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	int skl;
	std::ostringstream buffer;

	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("Чувак, укажи ИД проверяемого скилла.\r\n", ch);
		return;
	}
	skl = Skill::GetNumByID(std::string(argument));
	if (skl == SKILL_UNDEFINED)
	{
		send_to_char("Извини, братан, не нашел. :(\r\n", ch);
		return;
	}
	else {
		buffer << " Найден скилл " << skill_info[skl].name << " под номером " << skl << "\r\n";
	}

	send_to_char(buffer.str(), ch);
}

void do_loadstat(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	std::ifstream istream(LOAD_LOG_FOLDER LOAD_LOG_FILE, std::ifstream::in);
	int length;

	if (!istream.is_open())
	{
		send_to_char("Can't open file", ch);
		log("ERROR: Can't open file %s", LOAD_LOG_FOLDER LOAD_LOG_FILE);
		return;
	}

	istream.seekg(0, istream.end);
	length = istream.tellg();
	istream.seekg(0, istream.beg);
	istream.read(buf, MIN(length, MAX_STRING_LENGTH - 1));
	buf[istream.gcount()] = '\0';
	send_to_char(buf, ch);
}

namespace
{

	struct filter_type
	{
		filter_type() : type(-1), wear(EWearFlag::ITEM_WEAR_UNDEFINED), wear_message(-1), material(-1) {};

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

void do_print_armor(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch) || (!IS_GRGOD(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO)))
	{
		send_to_char("Чаво?\r\n", ch);
		return;
	}

	filter_type filter;
	char tmpbuf[MAX_INPUT_LENGTH];
	bool find_param = false;
	while (*argument)
	{
		switch (*argument)
		{
		case 'М':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "булат"))
			{
				filter.material = OBJ_DATA::MAT_BULAT;
			}
			else if (is_abbrev(tmpbuf, "бронза"))
			{
				filter.material = OBJ_DATA::MAT_BRONZE;
			}
			else if (is_abbrev(tmpbuf, "железо"))
			{
				filter.material = OBJ_DATA::MAT_IRON;
			}
			else if (is_abbrev(tmpbuf, "сталь"))
			{
				filter.material = OBJ_DATA::MAT_STEEL;
			}
			else if (is_abbrev(tmpbuf, "кованая.сталь"))
			{
				filter.material = OBJ_DATA::MAT_SWORDSSTEEL;
			}
			else if (is_abbrev(tmpbuf, "драг.металл"))
			{
				filter.material = OBJ_DATA::MAT_COLOR;
			}
			else if (is_abbrev(tmpbuf, "кристалл"))
			{
				filter.material = OBJ_DATA::MAT_CRYSTALL;
			}
			else if (is_abbrev(tmpbuf, "дерево"))
			{
				filter.material = OBJ_DATA::MAT_WOOD;
			}
			else if (is_abbrev(tmpbuf, "прочное.дерево"))
			{
				filter.material = OBJ_DATA::MAT_SUPERWOOD;
			}
			else if (is_abbrev(tmpbuf, "керамика"))
			{
				filter.material = OBJ_DATA::MAT_FARFOR;
			}
			else if (is_abbrev(tmpbuf, "стекло"))
			{
				filter.material = OBJ_DATA::MAT_GLASS;
			}
			else if (is_abbrev(tmpbuf, "камень"))
			{
				filter.material = OBJ_DATA::MAT_ROCK;
			}
			else if (is_abbrev(tmpbuf, "кость"))
			{
				filter.material = OBJ_DATA::MAT_BONE;
			}
			else if (is_abbrev(tmpbuf, "ткань"))
			{
				filter.material = OBJ_DATA::MAT_MATERIA;
			}
			else if (is_abbrev(tmpbuf, "кожа"))
			{
				filter.material = OBJ_DATA::MAT_SKIN;
			}
			else if (is_abbrev(tmpbuf, "органика"))
			{
				filter.material = OBJ_DATA::MAT_ORGANIC;
			}
			else if (is_abbrev(tmpbuf, "береста"))
			{
				filter.material = OBJ_DATA::MAT_PAPER;
			}
			else if (is_abbrev(tmpbuf, "драг.камень"))
			{
				filter.material = OBJ_DATA::MAT_DIAMOND;
			}
			else
			{
				send_to_char("Неверный материал предмета.\r\n", ch);
				return;
			}
			find_param = true;
			break;
		case 'Т':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "броня") || is_abbrev(tmpbuf, "armor"))
			{
				filter.type = OBJ_DATA::ITEM_ARMOR;
			}
			else if (is_abbrev(tmpbuf, "легкие") || is_abbrev(tmpbuf, "легкая"))
			{
				filter.type = OBJ_DATA::ITEM_ARMOR_LIGHT;
			}
			else if (is_abbrev(tmpbuf, "средние") || is_abbrev(tmpbuf, "средняя"))
			{
				filter.type = OBJ_DATA::ITEM_ARMOR_MEDIAN;
			}
			else if (is_abbrev(tmpbuf, "тяжелые") || is_abbrev(tmpbuf, "тяжелая"))
			{
				filter.type = OBJ_DATA::ITEM_ARMOR_HEAVY;
			}
			else
			{
				send_to_char("Неверный тип предмета.\r\n", ch);
				return;
			}
			find_param = true;
			break;
		case 'О':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "тело"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_BODY;
				filter.wear_message = 3;
			}
			else if (is_abbrev(tmpbuf, "голова"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_HEAD;
				filter.wear_message = 4;
			}
			else if (is_abbrev(tmpbuf, "ноги"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_LEGS;
				filter.wear_message = 5;
			}
			else if (is_abbrev(tmpbuf, "ступни"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_FEET;
				filter.wear_message = 6;
			}
			else if (is_abbrev(tmpbuf, "кисти"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_HANDS;
				filter.wear_message = 7;
			}
			else if (is_abbrev(tmpbuf, "руки"))
			{
				filter.wear = EWearFlag::ITEM_WEAR_ARMS;
				filter.wear_message = 8;
			}
			else
			{
				send_to_char("Неверное место одевания предмета.\r\n", ch);
				return;
			}
			find_param = true;
			break;
		case 'А':
		{
			bool tmp_find = false;
			argument = one_argument(++argument, tmpbuf);
			if (!strlen(tmpbuf))
			{
				send_to_char("Неверный аффект предмета.\r\n", ch);
				return;
			}
			if (filter.affect.size() + filter.affect2.size() + filter.affect3.size() >= 3)
			{
				break;
			}
			switch (*tmpbuf)
			{
			case '1':
				sprintf(tmpbuf, "можно вплавить 1 камень");
				break;
			case '2':
				sprintf(tmpbuf, "можно вплавить 2 камня");
				break;
			case '3':
				sprintf(tmpbuf, "можно вплавить 3 камня");
				break;
			default:
				break;
			}
			lower_convert(tmpbuf);
			size_t len = strlen(tmpbuf);
			int num = 0;

			for (int flag = 0; flag < 4; ++flag)
			{
				for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num)
				{
					if (strlen(weapon_affects[num]) < len)
						continue;
					if (!strncmp(weapon_affects[num], tmpbuf, len))
					{
						filter.affect.push_back(num);
						tmp_find = true;
						break;
					}
				}
				if (tmp_find)
				{
					break;
				}
				++num;
			}
			if (!tmp_find)
			{
				for (num = 0; *apply_types[num] != '\n'; ++num)
				{
					if (strlen(apply_types[num]) < len)
						continue;
					if (!strncmp(apply_types[num], tmpbuf, len))
					{
						filter.affect2.push_back(num);
						tmp_find = true;
						break;
					}
				}
			}
			// поиск по экстрафлагу
			if (!tmp_find)
			{
				num = 0;
				for (int flag = 0; flag < 4; ++flag)
				{
					for (/* тут ничего не надо */; *extra_bits[num] != '\n'; ++num)
					{
						if (strlen(extra_bits[num]) < len)
							continue;
						if (!strncmp(extra_bits[num], tmpbuf, len))
						{
							filter.affect3.push_back(num);
							tmp_find = true;
							break;
						}
					}
					if (tmp_find)
					{
						break;
					}
					num++;
				}
			}
			if (!tmp_find)
			{
				sprintf(buf, "Неверный аффект предмета: '%s'.\r\n", tmpbuf);
				send_to_char(buf, ch);
				return;
			}
			find_param = true;
			break;
		}
		default:
			++argument;
		}
	}
	if (!find_param)
	{
		send_to_char("Формат команды:\r\n"
			"   armor Т[броня|легкие|средние|тяжелые] О[тело|голова|ногиступни|кисти|руки] А[аффект] М[материал]\r\n", ch);
		return;
	}
	std::string buffer = "Выборка по следующим параметрам: ";
	if (filter.material >= 0)
	{
		buffer += material_name[filter.material];
		buffer += " ";
	}
	if (filter.type >= 0)
	{
		buffer += item_types[filter.type];
		buffer += " ";
	}
	if (filter.wear != EWearFlag::ITEM_WEAR_UNDEFINED)
	{
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (!filter.affect.empty())
	{
		for (const auto it : filter.affect)
		{
			buffer += weapon_affects[it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty())
	{
		for (const auto it : filter.affect2)
		{
			buffer += apply_types[it];
			buffer += " ";
		}
	}
	if (!filter.affect3.empty())
	{
		for (const auto it : filter.affect3)
		{
			buffer += extra_bits[it];
			buffer += " ";
		}
	}
	buffer += "\r\nСредний уровень мобов в зоне | внум предмета  | материал | имя предмета + аффекты если есть\r\n";
	send_to_char(buffer, ch);

	std::multimap<int /* zone lvl */, int /* obj rnum */> tmp_list;
	for (const auto i : obj_proto)
	{
		// материал
		if (filter.material >= 0 && filter.material != GET_OBJ_MATER(i))
		{
			continue;
		}
		// тип
		if (filter.type >= 0 && filter.type != GET_OBJ_TYPE(i))
		{
			continue;
		}
		// куда можно одеть
		if (filter.wear != EWearFlag::ITEM_WEAR_UNDEFINED
			&& !i->has_wear_flag(filter.wear))
		{
			continue;
		}
		// аффекты
		bool find = true;
		if (!filter.affect.empty())
		{
			for (std::vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
			{
				if (!CompareBits(i->get_affect_flags(), weapon_affects, *it))
				{
					find = false;
					break;
				}
			}
			// аффект не найден, продолжать смысла нет
			if (!find)
			{
				continue;
			}
		}

		if (!filter.affect2.empty())
		{
			for (std::vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it)
			{
				find = false;
				for (int k = 0; k < MAX_OBJ_AFFECT; ++k)
				{
					if (i->get_affected(k).location == *it)
					{
						find = true;
						break;
					}
				}
			}
			// доп.свойство не найдено, продолжать смысла нет
			if (!find)
			{
				continue;
			}
		}
		if (!filter.affect3.empty())
		{
			for (std::vector<int>::const_iterator it = filter.affect3.begin(); it != filter.affect3.end() && find; ++it)
			{
				//find = true;
				if (!CompareBits(GET_OBJ_EXTRA(i), extra_bits, *it))
				{
					find = false;
					break;
				}
			}
			// экстрафлаг не найден, продолжать смысла нет
			if (!find)
			{
				continue;
			}
		}

		if (find)
		{
			const auto vnum = i->get_vnum() / 100;
			for (auto nr = 0; nr < static_cast<zone_rnum>(zone_table.size()); nr++)
			{
				if (vnum == zone_table[nr].number)
				{
					tmp_list.insert(std::make_pair(zone_table[nr].mob_level, GET_OBJ_RNUM(i)));
				}
			}
		}
	}

	std::ostringstream out;
	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i)
	{
		const auto obj = obj_proto[i->second];
		out << "   "
			<< std::setw(2) << i->first << " | "
			<< std::setw(7) << obj->get_vnum() << " | "
			<< std::setw(14) << material_name[GET_OBJ_MATER(obj)] << " | "
			<< GET_OBJ_PNAME(obj, 0) << "\r\n";

		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			int drndice = obj->get_affected(i).location;
			int drsdice = obj->get_affected(i).modifier;
			if (drndice == APPLY_NONE || !drsdice)
			{
				continue;
			}
			sprinttype(drndice, apply_types, buf2);
			bool negative = false;
			for (int j = 0; *apply_negative[j] != '\n'; j++)
			{
				if (!str_cmp(buf2, apply_negative[j]))
				{
					negative = true;
					break;
				}
			}
			if (obj->get_affected(i).modifier < 0)
			{
				negative = !negative;
			}
			sprintf(buf, "   %s%s%s%s%s%d%s\r\n",
				CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM),
				negative ? " ухудшает на " : " улучшает на ", abs(drsdice), CCNRM(ch, C_NRM));
			out << "      |         |                | " << buf;
		}
	}
	if (!out.str().empty())
	{
		send_to_char(ch, "Всего найдено предметов: %lu\r\n\r\n", tmp_list.size());
		page_string(ch->desc, out.str());
	}
	else
	{
		send_to_char("Ничего не найдено.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
