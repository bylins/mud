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

#include "conf.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
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
#include "scripting.hpp"
#include "pugixml.hpp"
#include "sets_drop.hpp"
#include "fight.h"
#include "ext_money.hpp"
#include "noob.hpp"
#include "mail.h"
#include "mob_stat.hpp"

using std::ifstream;
using std::fstream;

// external vars
extern bool need_warn;
extern FILE *player_fl;

extern CHAR_DATA *character_list;
extern OBJ_DATA *object_list;
extern DESCRIPTOR_DATA *descriptor_list;
extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;
extern struct zone_data *zone_table;
extern char const *class_abbrevs[];
extern char const *kin_abbrevs[];
extern const char *weapon_affects[];
extern time_t boot_time;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern int shutdown_time;
extern struct player_index_element *player_table;
extern vector < OBJ_DATA * >obj_proto;
extern CHAR_DATA *mob_proto;
extern const char *Dirs[];
extern unsigned long int number_of_bytes_read;
extern unsigned long int number_of_bytes_written;
extern long max_id;
// for chars
extern const char *pc_class_types[];
extern struct spell_info_type spell_info[];
// for name auto-agree
extern void agree_name(CHAR_DATA * d, const char *immname, int immlev);
extern void disagree_name(CHAR_DATA * d, const char *immname, int immlev);
// privileges class
extern int reboot_uptime;
extern BanList *ban;
extern int check_dupes_host(DESCRIPTOR_DATA * d, bool autocheck = 0);
extern bool CompareBits(FLAG_DATA flags, const char *names[], int affect);
ACMD(do_recall);

// extern functions
int level_exp(CHAR_DATA * ch, int level);
void hcontrol_list_houses(CHAR_DATA * ch);
void appear(CHAR_DATA * ch);
void reset_zone(zone_rnum zone);
int parse_class(char arg);
extern CHAR_DATA *find_char(long n);
void rename_char(CHAR_DATA * ch, char *oname);
int _parse_name(char *arg, char *name);
int Valid_Name(char *name);
int reserved_word(const char *name);
int compute_armor_class(CHAR_DATA * ch);
int calc_loadroom(CHAR_DATA * ch, int bplace_mode = BIRTH_PLACE_UNDEFINED);
extern bool can_be_reset(zone_rnum zone);
extern int is_empty(zone_rnum zone_nr);
void list_feats(CHAR_DATA * ch, CHAR_DATA * vict, bool all_feats);
void list_skills(CHAR_DATA * ch, CHAR_DATA * vict);
void list_spells(CHAR_DATA * ch, CHAR_DATA * vict, int all_spells);
extern void NewNameShow(CHAR_DATA * ch);
extern void NewNameRemove(CHAR_DATA * ch);
extern void NewNameRemove(const std::string& name, CHAR_DATA * ch);
extern void print_rune_stats(CHAR_DATA *ch);

// local functions
int perform_set(CHAR_DATA * ch, CHAR_DATA * vict, int mode, char *val_arg);
void perform_immort_invis(CHAR_DATA * ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(CHAR_DATA * ch, char *rawroomstr, int trig);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(CHAR_DATA * ch, const int rnum = 0);
void do_stat_object(CHAR_DATA * ch, OBJ_DATA * j, const int virt = 0);//added by WorM virt при vstat'е 1 чтобы считалось реальное кол-во объектов в мире
void do_stat_character(CHAR_DATA * ch, CHAR_DATA * k, const int virt = 0);//added by WorM virt при vstat'е 1 чтобы считалось реальное кол-во мобов в мире
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(CHAR_DATA * ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_inspect);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(CHAR_DATA * ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
void print_zone_to_buf(char **bufptr, zone_rnum zone);
ACMD(do_show);
ACMD(do_set);
ACMD(do_liblist);
ACMD(do_name);
//
ACMD(do_godtest);
ACMD(do_sdemigod);
ACMD(do_unfreeze);
ACMD(do_setall);
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
    		    send_to_char(text, d->character);
    		}
    	    }
        }
}

#define MAX_TIME 0x7fffffff

extern const char *deaf_social;

// Adds karma string to KARMA
void add_karma(CHAR_DATA * ch, const char * punish , const char * reason)
{
	if (reason && (reason[0] != '.'))
	{
		time_t nt = time(NULL);
		sprintf(buf1, "%s :: %s [%s]\r\n", rustime(localtime(&nt)), punish, reason);
		KARMA(ch) = str_add(KARMA(ch), buf1);
	};
}



int set_punish(CHAR_DATA * ch, CHAR_DATA * vict, int punish , char * reason , long times)
{
	struct punish_data * pundata = 0;
	int result;
	if (ch == vict)
	{
		send_to_char("Это слишком жестоко...\r\n", ch);
		return 0;
	}

	if ((GET_LEVEL(vict) >= LVL_IMMORT && !IS_IMPL(ch)) || PRF_FLAGGED(vict, PRF_CODERINFO))
	{
		send_to_char("Кем вы себя возомнили?\r\n", ch);
		return 0;
	}

	// Проверяем а может ли чар вообще работать с этим наказанием.
	switch (punish)
	{
	case SCMD_MUTE:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->pmute));
		break;
	case SCMD_DUMB:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->pdumb));
		break;
	case SCMD_HELL:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->phell));
		break;
	case SCMD_NAME:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->pname));
		break;

	case SCMD_FREEZE:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->pfreeze));
		break;

	case SCMD_UNREGISTER:
		pundata = & CHECK_PLAYER_SPECIAL((vict), ((vict)->player_specials->punreg));
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
			REMOVE_BIT(PLR_FLAGS(vict, PLR_MUTE), PLR_MUTE);

			sprintf(buf, "Mute OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

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
			REMOVE_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
			Glory::remove_freeze(GET_UNIQUE(vict));

			sprintf(buf, "Freeze OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

			sprintf(buf, "Freeze OFF by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

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
			REMOVE_BIT(PLR_FLAGS(vict, PLR_DUMB), PLR_DUMB);

			sprintf(buf, "Dumb OFF for %s by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

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
			REMOVE_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);

			sprintf(buf, "%s removed FROM hell by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

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
			REMOVE_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);

			sprintf(buf, "%s removed FROM name room by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

			sprintf(buf, "Removed FROM name room by %s", GET_NAME(ch));
			add_karma(vict, buf, reason);

			if (IN_ROOM(vict) != NOWHERE)
			{

				act("$n выпущен$a из комнаты имени!", FALSE, vict, 0, 0, TO_ROOM);

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
			sprintf(buf, "%s%s выпустил$G вас из комнаты имени.%s",
					CCIGRN(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n выпущен$a из комнаты имени!");
			break;

		case SCMD_UNREGISTER:
			// Регистриуем чара
			if (PLR_FLAGGED(vict, PLR_REGISTERED))
			{
				send_to_char("Вашей жертва уже зарегистрирована.\r\n", ch);
				return (0);
			};

			sprintf(buf, "%s registered by %s.", GET_NAME(vict), GET_NAME(ch));
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

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
			SET_BIT(PLR_FLAGS(vict, PLR_MUTE), PLR_MUTE);
			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			sprintf(buf, "Mute ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

			sprintf(buf, "Mute ON (%ldh) by %s", times , GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s запретил$G вам кричать.%s",
					CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n подавился своим криком.");

			break;

		case SCMD_FREEZE:
			SET_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
			Glory::set_freeze(GET_UNIQUE(vict));
			pundata->duration = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;

			sprintf(buf, "Freeze ON for %s by %s(%ldh).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);
			sprintf(buf, "Freeze ON (%ldh) by %s", times , GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%sАдский холод сковал ваше тело ледяным панцирем.\r\n%s",
					CCIBLU(vict, C_NRM), CCNRM(vict, C_NRM));

			sprintf(buf2, "Ледяной панцирь покрыл тело $n1! Стало очень тихо и холодно.");

			break;


		case SCMD_DUMB:

			SET_BIT(PLR_FLAGS(vict, PLR_DUMB), PLR_DUMB);
			pundata->duration = (times > 0) ? time(NULL) + times * 60 : MAX_TIME;

			sprintf(buf, "Dumb ON for %s by %s(%ldm).", GET_NAME(vict), GET_NAME(ch), times);
			mudlog(buf, DEF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log(buf);

			sprintf(buf, "Dumb ON (%ldm) by %s", times , GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s запретил$G вам издавать звуки.%s",
					CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));

			sprintf(buf2, "$n дал$g обет молчания.");
			break;
		case SCMD_HELL:
			SET_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);

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
			imm_log(buf);
			sprintf(buf, "Moved TO hell (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s поместил$G вас в темницу.%s", GET_NAME(ch),
					CCIRED(vict, C_NRM), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n водворен$a в темницу!");
			break;

		case SCMD_NAME:

			SET_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);

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
			imm_log(buf);
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
			imm_log(buf);
			sprintf(buf, "Unregistered (%ldh) by %s", times, GET_NAME(ch));
			add_karma(vict, buf, reason);

			sprintf(buf, "%s%s снял$G с вас... регистрацию :).%s",
					CCIRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
			sprintf(buf2, "$n лишен$a регистрации!");

			break;

		}
	}
	if (IN_ROOM(ch) != NOWHERE)
	{
		act(buf, FALSE, vict, 0, ch, TO_CHAR);
		act(buf2, FALSE, vict, 0, ch, TO_ROOM);
	};
	return 1;
}

SetAllInspReqListType setall_inspect_list;
InspReqListType inspect_list;

#define SETALL_FREEZE 0
#define SETALL_EMAIL 1
#define SETALL_PSWD 2

void setall_inspect()
{
    if(setall_inspect_list.size() == 0)
		return;
    SetAllInspReqListType::iterator it = setall_inspect_list.begin();
    CHAR_DATA *ch = 0;
    DESCRIPTOR_DATA *d_vict = 0;
    DESCRIPTOR_DATA *imm_d = 0;
	if (!(imm_d = DescByUID(player_table[it->first].unique)) || (STATE(imm_d) != CON_PLAYING) || !(ch = imm_d->character))
	{
		setall_inspect_list.erase(it->first);
		return;
	}    
	struct timeval start, stop, result;
	int is_online;
	need_warn = false;
	gettimeofday(&start, NULL);
	Player *vict;
	for(;it->second->pos <= top_of_p_table; it->second->pos++)
	{
		vict = new Player;
		gettimeofday(&stop, NULL);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= OPT_USEC)
		{
			return;
		}
		buf1[0] = '\0';
		is_online = 0;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict)
			is_online = 1;
		if(player_table[it->second->pos].mail)
		    if(strstr(player_table[it->second->pos].mail, it->second->mail))
			{
				it->second->found++;
				if (it->second->type_req == SETALL_FREEZE)		
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							continue;
						}
						set_punish(imm_d->character, d_vict->character, SCMD_FREEZE, it->second->reason, it->second->freeze_time);
					}
					else
					{
						if (load_char(player_table[it->second->pos].name, vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки чара: %s\r\n", player_table[it->second->pos].name);
							delete vict;
							it->second->out += buf1;
							continue;
						}
						else
						{
							if (GET_LEVEL(vict) >= LVL_GOD)
							{
								printf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
								it->second->out += buf1;
								continue;
							}
							set_punish(imm_d->character, vict, SCMD_FREEZE, it->second->reason, it->second->freeze_time);
							vict->save_char();
						}
					}
				}
				if (it->second->type_req == SETALL_EMAIL)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							continue;
						}
						strncpy(GET_EMAIL(d_vict->character), it->second->newmail, 127);
						*(GET_EMAIL(d_vict->character) + 127) = '\0';
						sprintf(buf2, "Смена емайла у чара %s на %s\r\n", player_table[it->second->pos].name, it->second->newmail);
						sprintf(buf2, "Смена емайла на %s", it->second->newmail);
						add_karma(d_vict->character, buf2, GET_NAME(imm_d->character));
						it->second->out += buf2;
						
					}
					else
					{
						if (load_char(player_table[it->second->pos].name, vict) < 0)
						{
							printf("1");
							sprintf(buf1, "Ошибка загрузки чара: %s\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							delete vict;
							continue;
						}
						else
						{
							if (GET_LEVEL(vict) >= LVL_GOD)
							{
								printf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
								it->second->out += buf1;
								continue;
							}
							printf("6");
							strncpy(GET_EMAIL(vict), it->second->newmail, 127);
							*(GET_EMAIL(vict) + 127) = '\0';							
							sprintf(buf2, "Смена емайла у чара %s на %s\r\n", player_table[it->second->pos].name, it->second->newmail);
							it->second->out += buf2;
							sprintf(buf2, "Смена емайла на %s", it->second->newmail);
							add_karma(vict, buf2, GET_NAME(imm_d->character));
							vict->save_char();
						}
					}
				}
				
				if (it->second->type_req == SETALL_PSWD)
				{
					if (is_online)
					{
						if (GET_LEVEL(d_vict->character) >= LVL_GOD)
						{
							sprintf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							continue;
						}
						Password::set_password(d_vict->character, std::string(it->second->pwd));
						sprintf(buf1, "У чара %s изменен пароль на %s\r\n", player_table[it->second->pos].name, it->second->pwd);
						it->second->out += buf1;
						sprintf(buf2, "Изменен пароль", player_table[it->second->pos].name);
						add_karma(d_vict->character, buf2, GET_NAME(imm_d->character));
					}
					else
					{
						if (load_char(player_table[it->second->pos].name, vict) < 0)
						{
							sprintf(buf1, "Ошибка загрузки чара: %s\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							delete vict;
							continue;
						}
						if (GET_LEVEL(vict) >= LVL_GOD)
						{
							printf(buf1, "Чар %s бессмертный!\r\n", player_table[it->second->pos].name);
							it->second->out += buf1;
							continue;
						}
						Password::set_password(vict, std::string(it->second->pwd));
						sprintf(buf1, "У чара %s изменен пароль на %s\r\n", player_table[it->second->pos].name, it->second->pwd);
						it->second->out += buf1;
						sprintf(buf2, "Изменен пароль", player_table[it->second->pos].name);
						add_karma(d_vict->character, buf2, GET_NAME(imm_d->character));
						vict->save_char();
					}
				}							
			}
		delete vict;	
	}
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

ACMD(do_setall)
{
	int type_request = 0;
	int times = 0;
	if (ch->get_pfilepos() < 0)
		return;
	
	InspReqListType::iterator it_inspect = inspect_list.find(GET_UNIQUE(ch));
	SetAllInspReqListType::iterator it = setall_inspect_list.find(GET_UNIQUE(ch));
	// // Навсякий случай разрешаем только одну команду такого типа, либо сетол, либо инспект
	if (it_inspect != inspect_list.end() and it != setall_inspect_list.end())
	{
		send_to_char(ch, "Обрабатывается другой запрос, подождите...\r\n", argument);
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
		send_to_char("Usage: setall { email } [command (email pswd frozen)] [для frozen время] <argument> \r\n", ch);
		return;
	}
	
	if (!valid_email(buf))
	{
		send_to_char("Некорректный E-mail !\r\n", ch);
		return;
	}
	
	if (!isname(buf1, "frozen email pswd"))
	{
		send_to_char("Сделай лучше что-нибудь нормальное.\r\n", ch);
		return;
	}
	if (is_abbrev(buf1, "frozen"))
	{	
		skip_spaces(&argument);
		if (!argument || !*argument)
		{
			send_to_char("И по какой причине Вы решиле ЭТО сделать?\r\n", ch);
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
			send_to_char("Новый e-mail невалидный !\r\n", ch);
			return;
		}
		req->newmail = strdup(buf2);
		type_request = SETALL_EMAIL;
	}
	else if (is_abbrev(buf1, "pswd"))
	{
		if (!*buf2)
		{
			send_to_char("Укажите новый пароль!\r\n", ch);
			return;
		}
		req->pwd = strdup(buf2);
		type_request = SETALL_PSWD;
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



ACMD(do_echo)
{
	CHAR_DATA *to;

	if (PLR_FLAGGED(ch, PLR_DUMB))
	{
		send_to_char("Вы не в состоянии что-либо продемонстрировать окружающим.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument)
		send_to_char("И что вы хотите выразить столь красочно?\r\n", ch);
	else
	{
		if (subcmd == SCMD_EMOTE)
		{
			// added by Pereplut
			if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
			{
				if PLR_FLAGGED(ch->master, PLR_DUMB)
				{
// shapirus: правильно пишется не "так-же", а "так же".
// и запятая пропущена была :-P.
					send_to_char("Ваши последователи так же немы, как и вы!\r\n", ch->master);
					return;
				}
			}
			sprintf(buf, "&K$n %s.&n", argument);
		}
		else
			strcpy(buf, argument);
		for (to = world[ch->in_room]->people; to; to = to->next_in_room)
		{
			if (to == ch || ignores(to, ch, IGNORE_EMOTE))
				continue;
			act(buf, FALSE, ch, 0, to, TO_VICT | CHECK_DEAF);
			act(deaf_social, FALSE, ch, 0, to, TO_VICT | CHECK_NODEAF);
		}
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
			act(buf, FALSE, ch, 0, 0, TO_CHAR);
	}
}

#define SHOW_GLORY 	0
#define ADD_GLORY 	1
#define SUB_GLORY 	2
#define SUB_STATS 	3
#define SUB_TRANS 	4
#define SUB_HIDE    5

ACMD(do_glory)
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

ACMD(do_send)
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
	if (isdigit(*roomstr) && !strchr(roomstr, '.'))
	{
		tmp = atoi(roomstr);
		if ((location = real_room(tmp)) == NOWHERE)
		{
			send_to_char("Нет комнаты с таким номером.\r\n", ch);
			return (NOWHERE);
		}
	}
	else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) != NULL)
		location = target_mob->in_room;
	else if ((target_obj = get_obj_vis(ch, roomstr)) != NULL)
	{
		if (target_obj->in_room != NOWHERE)
			location = target_obj->in_room;
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



ACMD(do_at)
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

ACMD(do_unfreeze)
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
    
    delete [] reason_c;
    unfreeze_list.close();
    
}

ACMD(do_goto)
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

ACMD(do_teleport)
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
		act("$n переместил$g вас!", FALSE, ch, 0, (char *) victim, TO_VICT);
		look_at_room(victim, 0);
	}
}



ACMD(do_vnum)
{
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj")
							&& !is_abbrev(buf, "flag")))
	{
		send_to_char("Usage: vnum { obj | mob | flag } <name>\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "mob"))
		if (!vnum_mobile(buf2, ch))
			send_to_char("Нет существа с таким именем.\r\n", ch);

	if (is_abbrev(buf, "obj"))
		if (!vnum_object(buf2, ch))
			send_to_char("Нет предмета с таким названием.\r\n", ch);

	if (is_abbrev(buf, "flag"))
		if (!vnum_flag(buf2, ch))
			send_to_char("Нет объектов с таким флагом.\r\n", ch);
}



void do_stat_room(CHAR_DATA * ch, const int rnum)
{
	EXTRA_DESCR_DATA *desc;
	ROOM_DATA *rm = world[ch->in_room];
	int i, found;
	OBJ_DATA *j;
	CHAR_DATA *k;
	AFFECT_DATA *aff;
	if(rnum != 0)
		rm = world[rnum];

	sprintf(buf, "Комната : %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprinttype(rm->sector_type, sector_types, buf2);
	sprintf(buf,
			"Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
			zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), ch->in_room, buf2);
	send_to_char(buf, ch);

	sprintbits(rm->room_flags, room_bits, buf2, ",");
	sprintf(buf, "СпецПроцедура: %s, Флаги: %s\r\n", (rm->func == NULL) ? "None" : "Exists", buf2);
	send_to_char(buf, ch);

	send_to_char("Описание:\r\n", ch);
	send_to_char(RoomDescription::show_desc(rm->description_num), ch);

	if (rm->ex_description)
	{
		sprintf(buf, "Доп. описание:%s", CCCYN(ch, C_NRM));
		for (desc = rm->ex_description; desc; desc = desc->next)
		{
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		send_to_char(strcat(buf, "\r\n"), ch);
	}
	sprintf(buf, "Живые существа:%s", CCYEL(ch, C_NRM));
	for (found = 0, k = rm->people; k; k = k->next_in_room)
	{
		if (!CAN_SEE(ch, k))
			continue;
		sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
				(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
		strcat(buf, buf2);
		if (strlen(buf) >= 62)
		{
			if (k->next_in_room)
				send_to_char(strcat(buf, ",\r\n"), ch);
			else
				send_to_char(strcat(buf, "\r\n"), ch);
			*buf = found = 0;
		}
	}

	if (*buf)
		send_to_char(strcat(buf, "\r\n"), ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (rm->contents)
	{
		sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
		for (found = 0, j = rm->contents; j; j = j->next_content)
		{
			if (!CAN_SEE_OBJ(ch, j))
				continue;
			sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
			strcat(buf, buf2);
			if (strlen(buf) >= 62)
			{
				if (j->next_content)
					send_to_char(strcat(buf, ",\r\n"), ch);
				else
					send_to_char(strcat(buf, "\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			send_to_char(strcat(buf, "\r\n"), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		if (rm->dir_option[i])
		{
			if (rm->dir_option[i]->to_room == NOWHERE)
				sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			else
				sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
						GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));
			sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
			sprintf(buf,
					"Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s (%s), Тип: %s\r\n ",
					CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
					rm->dir_option[i]->key,
					rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
					rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", buf2);
			send_to_char(buf, ch);
			if (rm->dir_option[i]->general_description)
				strcpy(buf, rm->dir_option[i]->general_description);
			else
				strcpy(buf, "  Нет описания выхода.\r\n");
			send_to_char(buf, ch);
		}
	}
	if (rm->affected)
	{
		sprintf(buf1," Аффекты на комнате:\r\n");
		for (aff = rm->affected; aff; aff = aff->next)
		{
			sprintf(buf1+strlen(buf1),
				"       Заклинание \"%s\" (%d) - %s.\r\n",
			spell_name(aff->type),
			aff->duration,
			((k = find_char(aff->caster_id)) ? GET_NAME(k) : "неизвестно"));
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
	EXTRA_DESCR_DATA *desc;
	long int li;
	bool is_grgod = IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO) ? true : false;

	vnum = GET_OBJ_VNUM(j);
	rnum = GET_OBJ_RNUM(j);
	sprintf(buf, "Название: '%s%s%s',\r\nСинонимы: &S%s&s\r\n",
			CCYEL(ch, C_NRM),
			((j->short_description) ? j->short_description : "<None>"), CCNRM(ch, C_NRM), j->aliases);
	send_to_char(buf, ch);
	sprinttype(GET_OBJ_TYPE(j), item_types, buf1);

	if (GET_OBJ_RNUM(j) >= 0)
		strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Есть" : "Нет"));
	else
		strcpy(buf2, "None");

	send_to_char(ch, "VNum: [%s%5d%s], RNum: [%5d], UID: [%d]\r\n",
		CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), GET_OBJ_UID(j));

	send_to_char(ch, "Тип: %s, СпецПроцедура: %s", buf1, buf2);

	if (GET_OBJ_OWNER(j))
	{
		send_to_char(ch, ", Владелец : %s", get_name_by_unique(GET_OBJ_OWNER(j)));
	}
	if (GET_OBJ_MAKER(j))
	{
		send_to_char(ch, ", Создатель : %s", get_name_by_unique(GET_OBJ_MAKER(j)));
	}
	if (GET_OBJ_PARENT(j))
	{
		send_to_char(ch, ", Родитель(VNum) : [%d]", GET_OBJ_PARENT(j));
	}

	sprintf(buf, "\r\nL-Des: %s\r\n%s", ((j->description) ? j->description : "Нет"), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	if (j->ex_description)
	{
		sprintf(buf, "Экстра описание:%s", CCCYN(ch, C_NRM));
		for (desc = j->ex_description; desc; desc = desc->next)
		{
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		send_to_char(strcat(buf, "\r\n"), ch);
	}
	send_to_char("Может быть надет : ", ch);
	sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	sprintf(buf, "Материал : ");
	sprinttype(j->obj_flags.Obj_mater, material_name, buf+strlen(buf));
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Неудобства : ", ch);
	sprintbits(j->obj_flags.no_flag, no_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Запреты : ", ch);
	sprintbits(j->obj_flags.anti_flag, anti_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Устанавливает аффекты : ", ch);
	sprintbits(j->obj_flags.affects, weapon_affects, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	send_to_char("Дополнительные флаги  : ", ch);
	sprintbits(j->obj_flags.extra_flags, extra_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	sprintf(buf,
			"Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, Таймер: %d\r\n",
			GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTEQ(j), GET_OBJ_RENT(j), j->get_timer());
	send_to_char(buf, ch);

	strcpy(buf, "Находится : ");
	if ((j->in_room == NOWHERE) || !is_grgod)
		strcat(buf, "нигде");
	else
	{
		sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
		strcat(buf, buf2);
	}
	// NOTE: In order to make it this far, we must already be able to see the
	//       character holding the object. Therefore, we do not need CAN_SEE().
	strcat(buf, ", В контейнере: ");
	strcat(buf, (j->in_obj && is_grgod) ? j->in_obj->short_description : "Нет");
	strcat(buf, ", В инвентаре: ");
	strcat(buf, (j->carried_by && is_grgod) ? GET_NAME(j->carried_by) : "Нет");
	strcat(buf, ", Надет: ");
	strcat(buf, (j->worn_by && is_grgod) ? GET_NAME(j->worn_by) : "Нет");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	switch (GET_OBJ_TYPE(j))
	{
	case ITEM_LIGHT:
		if (GET_OBJ_VAL(j, 2) < 0)
			strcpy(buf, "Вечный свет!");
		else
			sprintf(buf, "Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		sprintf(buf, "Заклинания: (Уровень %d) %s, %s, %s",
				GET_OBJ_VAL(j, 0),
				spell_name(GET_OBJ_VAL(j, 1)), spell_name(GET_OBJ_VAL(j, 2)), spell_name(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
				spell_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_WEAPON:
		sprintf(buf, "Повреждения: %dd%d, Тип повреждения: %d",
				GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		sprintf(buf, "AC: [%d]  Броня: [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_TRAP:
		sprintf(buf, "Spell: %d, - Hitpoints: %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_CONTAINER:
		sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
		sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Труп: %s",
				GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2), YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
	{
		sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
		std::string spells = drinkcon::print_spells(ch, j);
		boost::trim(spells);
		sprintf(buf,
			"Обьем: %d, Содержит: %d, Отравлен: %s, Жидкость: %s\r\n%s",
			GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
			buf2, spells.c_str());
		break;
	}
	case ITEM_NOTE:
		sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
		break;
	case ITEM_KEY:
		strcpy(buf, "");
		break;
	case ITEM_FOOD:
		sprintf(buf, "Насыщает(час): %d, Отравлен: %s", GET_OBJ_VAL(j, 0), YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_MONEY:
		sprintf(buf, "Монет: %d", GET_OBJ_VAL(j, 0));
		break;

	case ITEM_INGRADIENT:
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
					CCICYN(ch, C_NRM), obj_proto[i]->PNames[0], CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;

	default:
		sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
				GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	}
	send_to_char(strcat(buf, "\r\n"), ch);

	// * I deleted the "equipment status" code from here because it seemed
	// * more or less useless and just takes up valuable screen space.

	if (j->contains)
	{
		sprintf(buf, "\r\nСодержит:%s", CCGRN(ch, C_NRM));
		for (found = 0, j2 = j->contains; j2; j2 = j2->next_content)
		{
			sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
			strcat(buf, buf2);
			if (strlen(buf) >= 62)
			{
				if (j2->next_content)
					send_to_char(strcat(buf, ",\r\n"), ch);
				else
					send_to_char(strcat(buf, "\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			send_to_char(strcat(buf, "\r\n"), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	found = 0;
	send_to_char("Аффекты:", ch);
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (j->affected[i].modifier)
		{
			sprinttype(j->affected[i].location, apply_types, buf2);
			sprintf(buf, "%s %+d to %s", found++ ? "," : "", j->affected[i].modifier, buf2);
			send_to_char(buf, ch);
		}
	if (!found)
		send_to_char(" Нет", ch);

	if (j->has_skills())
	{
		std::map<int, int> skills;
		j->get_skills(skills);
		int skill_num;
		int percent;

		send_to_char("\r\nУмения :", ch);
		for (std::map<int, int>::iterator it = skills.begin(); it != skills.end(); ++it)
		{
			skill_num = it->first;
			percent = it->second;

			if (percent == 0) // TODO: такого не должно быть?
				continue;

			sprintf(buf, " %+d%% to %s",
					percent,
					skill_info[skill_num].name);
			send_to_char(buf, ch);
		}
	}
	send_to_char("\r\n", ch);

	if (j->get_ilevel() > 0)
	{
		send_to_char(ch, "Уровень (ilvl): %d\r\n", j->get_ilevel());
	}

	if (j->get_manual_mort_req() >= 0)
	{
		send_to_char(ch, "Проставлено поле минимальных перевоплощений: %d\r\n", j->get_manual_mort_req());
	}
	else if (j->get_mort_req() > 0)
	{
		send_to_char(ch, "Требует перевоплощений: %d\r\n", j->get_mort_req());
	}

	if (is_grgod)
	{
		sprintf(buf, "Сейчас в мире : %d. На постое : %d\r\n",
				rnum >= 0 ? obj_index[rnum].number - (virt ? 1 : 0) : -1, rnum >= 0 ? obj_index[rnum].stored : -1);
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
	AFFECT_DATA *aff;

	int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);
	int k_room = -1;
	if (!virt && (god_level == LVL_IMPL || (god_level == LVL_GRGOD && !IS_NPC(k))))
		k_room = GET_ROOM_VNUM(IN_ROOM(k));

	sprinttype(GET_SEX(k), genders, buf);
	sprintf(buf2, " %s '%s' IDNum: [%ld] В комнате [%d] Текущий ID:[%ld]\r\n",
			(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
			GET_NAME(k), GET_IDNUM(k), k_room, GET_ID(k));
	send_to_char(strcat(buf, buf2), ch);
	if (IS_MOB(k))
	{
		sprintf(buf, "Синонимы: &S%s&s, VNum: [%5d], RNum: [%5d]\r\n",
				k->get_pc_name(), GET_MOB_VNUM(k), GET_MOB_RNUM(k));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Падежи: %s/%s/%s/%s/%s/%s ",
			GET_PAD(k, 0), GET_PAD(k, 1), GET_PAD(k, 2), GET_PAD(k, 3), GET_PAD(k, 4), GET_PAD(k, 5));
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

		sprintf(buf, "Вероисповедание: %s\r\n", religion_name[(int) GET_RELIGION(k)][(int) GET_SEX(k)]);
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
					(FREEZE_DURATION(k) - time(NULL)) / 3600, FREEZE_REASON(k) ? FREEZE_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_HELLED) && HELL_DURATION(k))
		{
			sprintf(buf, "Находится в темнице : %ld час [%s].\r\n",
					(HELL_DURATION(k) - time(NULL)) / 3600, HELL_REASON(k) ? HELL_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_NAMED) && NAME_DURATION(k))
		{
			sprintf(buf, "Находится в комнате имени : %ld час.\r\n",
					(NAME_DURATION(k) - time(NULL)) / 3600);
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_MUTE) && MUTE_DURATION(k))
		{
			sprintf(buf, "Будет молчать : %ld час [%s].\r\n",
					(MUTE_DURATION(k) - time(NULL)) / 3600, MUTE_REASON(k) ? MUTE_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (PLR_FLAGGED(k, PLR_DUMB) && DUMB_DURATION(k))
		{
			sprintf(buf, "Будет нем : %ld мин [%s].\r\n",
					(DUMB_DURATION(k) - time(NULL)) / 60, DUMB_REASON(k) ? DUMB_REASON(k) : "-");
			send_to_char(buf, ch);
		}
		if (!PLR_FLAGGED(k, PLR_REGISTERED) && UNREG_DURATION(k))
		{
			sprintf(buf, "Не будет зарегестрирован : %ld час [%s].\r\n",
					(UNREG_DURATION(k) - time(NULL)) / 3600, UNREG_REASON(k) ? UNREG_REASON(k) : "-");
			send_to_char(buf, ch);
		}

		if (GET_GOD_FLAG(k, GF_GODSLIKE) && GCURSE_DURATION(k))
		{
			sprintf(buf, "Под защитой Богов : %ld час.\r\n", (GCURSE_DURATION(k) - time(NULL)) / 3600);
			send_to_char(buf, ch);
		}
		if (GET_GOD_FLAG(k, GF_GODSCURSE) && GCURSE_DURATION(k))
		{
			sprintf(buf, "Проклят Богами : %ld час.\r\n", (GCURSE_DURATION(k) - time(NULL)) / 3600);
			send_to_char(buf, ch);
		}
	}

	sprintf(buf, "Титул: %s\r\n", (k->player_data.title ? k->player_data.title : "<Нет>"));
	send_to_char(buf, ch);
	if (IS_NPC(k))
		sprintf(buf, "L-Des: %s", (k->player_data.long_descr ? k->player_data.long_descr : "<Нет>\r\n"));
	else
		sprintf(buf, "L-Des: %s", (k->player_data.description ? k->player_data.description : "<Нет>\r\n"));
	send_to_char(buf, ch);

	if (!IS_NPC(k))
	{
		sprinttype(k->get_class(), pc_class_types, buf2);
		sprintf(buf, "Племя: %s, Род: %s, Профессия: %s",
			string(PlayerRace::GetKinNameByNum(GET_KIN(k),GET_SEX(k))).c_str(),
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
		sprintf(buf, "Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld), Гривны: %d/%d/%d %s",
			GET_LOADROOM(k), k->get_gold(), k->get_bank(), k->get_total_gold(),
			k->get_ext_money(ExtMoney::TORC_GOLD),
			k->get_ext_money(ExtMoney::TORC_SILVER),
			k->get_ext_money(ExtMoney::TORC_BRONZE),
			ExtMoney::draw_daily_limit(k, true).c_str());

		//. Display OLC zone for immorts .
		if (GET_LEVEL(k) >= LVL_IMMORT)
		{
			sprintf(buf1, ", OLC[%d]", GET_OLC_ZONE(k));
			strcat(buf, buf1);
		}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
	else
	{
		sprintf(buf, "Сейчас в мире : %d. \r\n", GET_MOB_RNUM(k) >= 0 ? mob_index[GET_MOB_RNUM(k)].number - (virt ? 1 : 0) : -1);
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
			"Glory: [%d], ConstGlory: [%d], AC: [%d/%d(%d)], Броня: [%d], Hitroll: [%2d/%2d/%d], Damroll: [%2d/%2d/%d]\r\n",
			Glory::get_glory(GET_UNIQUE(k)), GloryConst::get_glory(GET_UNIQUE(k)), GET_AC(k), GET_REAL_AC(k),
			compute_armor_class(k), GET_ARMOUR(k), GET_HR(k),
			GET_REAL_HR(k), GET_REAL_HR(k) + str_bonus(GET_REAL_STR(k), STR_TO_HIT),
			GET_DR(k), GET_REAL_DR(k), GET_REAL_DR(k) + str_bonus(GET_REAL_STR(k), STR_TO_DAM));
	send_to_char(buf, ch);
	sprintf(buf,
			"Saving throws: [Para:%d/Breath:%d/Spell:%d/Basic:%d], Morale: [%d], Init: [%d], ToCast: [%d]\r\n",
			GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3),
			k->calc_morale(), GET_INITIATIVE(k), GET_CAST_SUCCESS(k));
	send_to_char(buf, ch);
	sprintf(buf,
			"Resistances: [Fire:%d/Air:%d/Water:%d/Earth:%d/Vit:%d/Mind:%d/Immun:%d]\r\n",
			GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
			GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6));
	send_to_char(buf, ch);
	sprintf(buf,
			"Magic affect resist : [%d], Magic damage resist : [%d], Physical damage resist : [%d]\r\n", GET_AR(k), GET_MR(k), GET_PR(k));
	send_to_char(buf, ch);

	sprintf(buf, "EffCha: [%f], PlusMem: [%d], HpReg: [%d], MoveReg: [%d], Absorbe: [%d]\r\n",
			get_effective_cha(k, 0), GET_MANAREG(k), GET_HITREG(k), GET_MOVEREG(k), GET_ABSORBE(k));
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
		sprintbits(k->char_specials.saved.act, action_bits, buf2, ",");
		sprintf(buf, "NPC флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		sprintbits(k->mob_specials.npc_flags, function_bits, buf2, ",");
		sprintf(buf, "MOB флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}
	else
	{
		sprintbits(k->char_specials.saved.act, player_bits, buf2, ",");
		sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);

		sprintbits(k->player_specials->saved.pref, preference_bits, buf2, ",");
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

	for (i = 0, j = k->carrying; j; j = j->next_content, i++);
	sprintf(buf + strlen(buf), "(в инвентаре) : %d, ", i);

	for (i = 0, i2 = 0; i < NUM_WEARS; i++)
		if (GET_EQ(k, i))
			i2++;
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
		sprintf(buf, "Ведущий: %s, Ведомые:", ((k->master) ? GET_NAME(k->master) : "<нет>"));

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
	sprintbits(k->char_specials.saved.affected_by, affected_bits, buf2, ",");
	sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	// Routine to show what spells a char is affected by
	if (k->affected)
	{
		for (aff = k->affected; aff; aff = aff->next)
		{
			*buf2 = '\0';
			sprintf(buf, "Заклинания: (%3dsec) %s%-21s%s ", aff->duration + 1,
					CCCYN(ch, C_NRM), spell_name(aff->type), CCNRM(ch, C_NRM));
			if (aff->modifier)
			{
				sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
				strcat(buf, buf2);
			}
			if (aff->bitvector)
			{
				if (*buf2)
					strcat(buf, ", sets ");
				else
					strcat(buf, "sets ");
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
				sprintf(buf, "%10ld - %10ld\r\n", memchar->id, memchar->time - time(NULL));
				send_to_char(buf, ch);
			}
		}
		if (SCRIPT_MEM(k))
		{
			struct script_memory *mem = SCRIPT_MEM(k);
			send_to_char("Память (скрипт):\r\n  Игрок                Команда\r\n", ch);
			while (mem)
			{
				CHAR_DATA *mc = find_char(mem->id);
				if (!mc)
					send_to_char("  ** Разрушено!\r\n", ch);
				else
				{
					if (mem->cmd)
						sprintf(buf, "  %-20.20s%s\r\n", GET_NAME(mc), mem->cmd);
					else
						sprintf(buf, "  %-20.20s <default>\r\n", GET_NAME(mc));
					send_to_char(buf, ch);
				}
				mem = mem->next;
			}
		}
	}
	else  		// this is a PC, display their global variables
	{
		if (k->script && k->script->global_vars)
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
		log("Start logon list stat");
		// Отображаем список ip-адресов с которых персонаж входил
		if (LOGON_LIST(k))
		{
			struct logon_data * cur_log = LOGON_LIST(k);
			// update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
			// заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
			std::string out = "Персонаж заходил с IP-адресов:\r\n";
			while (cur_log)
			{
				sprintf(buf1, "%16s %5ld %20s \r\n", cur_log->ip, cur_log->count, rustime(localtime(&cur_log->lasttime)));
				out += buf1;
				cur_log = cur_log->next;
			}
			page_string(ch->desc, out);
		}
		log("End logon list stat");
	}
}


ACMD(do_stat)
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


ACMD(do_shutdown)
{
	static char const *help =
		"Формат команды shutdown [reboot|die|pause] кол-во секунд\r\n"
		"               shutdown schedule кол-во минут\r\n"
		"               shutdown now|cancel|schedule";

	two_arguments(argument, arg, buf);

	if (!*arg)
	{
		send_to_char(help, ch);
		return;
	}

	int times = 0;
	if (*buf)
	{
		times = atoi(buf);
	}

	if (!str_cmp(arg, "reboot") && times > 0)
	{
		times = MAX(30, times);
		sprintf(buf, "[ПЕРЕЗАГРУЗКА через %d %s]\r\n", times, desc_count(times, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Reboot by %s.", GET_NAME(ch));
		imm_log("Reboot by %s.", GET_NAME(ch));
		touch(FASTBOOT_FILE);
		circle_shutdown = 2;
		circle_reboot = 1;
		shutdown_time = time(0) + times;
		return;
	}
	else if (!str_cmp(arg, "die") && times > 0)
	{
		times = MAX(30, times);
		sprintf(buf, "[ОСТАНОВКА через %d %s]\r\n", times, desc_count(times, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Shutdown die by %s.", GET_NAME(ch));
		imm_log("Shutdown die by %s.", GET_NAME(ch));
		touch(KILLSCRIPT_FILE);
		circle_shutdown = 2;
		circle_reboot = 0;
		shutdown_time = time(0) + times;
		return;
	}
	else if (!str_cmp(arg, "pause") && times > 0)
	{
		times = MAX(30, times);
		sprintf(buf, "[ОСТАНОВКА через %d %s]\r\n", times, desc_count(times, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Shutdown pause by %s.", GET_NAME(ch));
		imm_log("Shutdown pause by %s.", GET_NAME(ch));
		touch(PAUSE_FILE);
		circle_shutdown = 2;
		circle_reboot = 0;
		shutdown_time = time(0) + times;
		return;
	}
	else if (!str_cmp(arg, "now"))
	{
		sprintf(buf, "(GC) Shutdown NOW by %s.", GET_NAME(ch));
		log("%s",buf);
		imm_log("Shutdown NOW by %s.", GET_NAME(ch));
		send_to_all("ПЕРЕЗАГРУЗКА.. Вернетесь через пару минут.\r\n");
		circle_shutdown = 1;
		circle_reboot = 2;
		shutdown_time = 0;
		return;
	}
	else if (!str_cmp(arg, "schedule"))
	{
		if (times <= 0)
		{
			time_t tmp_time = boot_time + (time_t)(60 * reboot_uptime);
			send_to_char(ch, "Сервер будет автоматически перезагружен в %s\r\n",
				rustime(localtime(&tmp_time)));
			return;
		}

		time_t uptime = time(0) - boot_time;
		reboot_uptime = uptime / 60 + times;
		circle_shutdown = 0;
		circle_reboot = 0;
		shutdown_time = 0;

		time_t tmp_time = boot_time + (time_t)(60 * reboot_uptime);
		send_to_char(ch, "Сервер будет автоматически перезагружен в %s\r\n",
			rustime(localtime(&tmp_time)));
		log("(GC) Shutdown scheduled by %s.", GET_NAME(ch));
		imm_log("Shutdown scheduled by %s.", GET_NAME(ch));
		return;
	}
	else if (!str_cmp(arg, "cancel"))
	{
		log("(GC) Shutdown canceled by %s.", GET_NAME(ch));
		imm_log("Shutdown canceled by %s.", GET_NAME(ch));
		send_to_all("ПЕРЕЗАГРУЗКА ОТМЕНЕНА.\r\n");

		circle_reboot = 0;
		circle_shutdown = 0;
		shutdown_time = 0;
		return;
	}
	send_to_char(help, ch);
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


ACMD(do_snoop)
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
			tch = victim->desc->original;
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



ACMD(do_switch)
{
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch->desc->original)
		send_to_char("Вы уже в чьем-то теле.\r\n", ch);
	else if (!*arg)
		send_to_char("Стать кем?\r\n", ch);
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("Нет такого создания.\r\n", ch);
	else if (ch == victim)
		send_to_char("Вы и так им являетесь.\r\n", ch);
	else if (victim->desc)
		send_to_char("Это тело уже под контролем.\r\n", ch);
	else if (!IS_IMPL(ch) && !IS_NPC(victim))
		send_to_char("Вы не столь могущественны, чтобы контроолировать тело игрока.\r\n", ch);
	else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
		send_to_char("Вы не можете находиться в той комнате.\r\n", ch);
	else if (!IS_GRGOD(ch) && !Clan::MayEnter(ch, IN_ROOM(victim), HCE_PORTAL))
		send_to_char("Вы не сможете проникнуть на частную территорию.\r\n", ch);
	else
	{
		send_to_char(OK, ch);

		ch->desc->character = victim;
		ch->desc->original = ch;

		victim->desc = ch->desc;
		ch->desc = NULL;
	}
}


ACMD(do_return)
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

ACMD(do_load)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	mob_vnum number;
	mob_rnum r_num;
	char *iname;

	iname = two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !isdigit(*buf2))
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
		obj = read_object(r_num, REAL);
		GET_OBJ_MAKER(obj) = GET_UNIQUE(ch);

		if (load_into_inventory)
			obj_to_char(obj, ch);
		else
			obj_to_room(obj, ch->in_room);
		act("$n покопал$u в МУДе.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n создал$g $o3!", FALSE, ch, obj, 0, TO_ROOM);
		act("Вы создали $o3.", FALSE, ch, obj, 0, TO_CHAR);
		load_otrigger(obj);
		obj_decay(obj);
		olc_log("%s load obj %s #%d", GET_NAME(ch), obj->short_description, number);
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
		obj = load_ingredient(i, power, power);
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
		olc_log("%s load ing %s #%d", GET_NAME(ch), obj->short_description, power);
	}
	else
		send_to_char("Нет уж. Ты создай че-нить нормальное.\r\n", ch);
}



ACMD(do_vstat)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	mob_vnum number;	// or obj_vnum ...
	mob_rnum r_num;		// or obj_rnum ...

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !isdigit(*buf2))
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
		obj = read_object(r_num, REAL);
		do_stat_object(ch, obj, 1);
		extract_obj(obj);
	}
	else
		send_to_char("Тут должно быть что-то типа 'obj' или 'mob'.\r\n", ch);
}




// clean a room of all mobiles and objects
ACMD(do_purge)
{
	CHAR_DATA *vict, *next_v;
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
			if (vict->followers || vict->master)
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

		for (vict = world[ch->in_room]->people; vict; vict = next_v)
		{
			next_v = vict->next_in_room;
			if (IS_NPC(vict))
			{
				if (vict->followers || vict->master)
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
			next_o = obj->next_content;
			extract_obj(obj);
		}
	}
}

const int IIP   = 1;
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
	if(inspect_list.size() == 0)
		return;

	InspReqListType::iterator it = inspect_list.begin();

	CHAR_DATA *ch = 0;
	DESCRIPTOR_DATA *d_vict = 0;

	//если нет дескриптора или он где-то там по меню шарица, то запрос отменяется
	if (!(d_vict = DescByUID(player_table[it->first].unique)) || (STATE(d_vict) != CON_PLAYING) || !(ch = d_vict->character))
	{
		inspect_list.erase(it->first);
		return;
	}

	struct timeval start, stop, result;
	time_t mytime;
	CHAR_DATA *vict = 0;
	int mail_found = 0;
	int is_online;
	need_warn = false;

	gettimeofday(&start, NULL);
	for(;it->second->pos <= top_of_p_table; it->second->pos++)
	{
		gettimeofday(&stop, NULL);
		timediff(&result, &stop, &start);
		if (result.tv_sec > 0 || result.tv_usec >= OPT_USEC)
		{
			return;
		}
		#ifdef TEST_BUILD
		log("inspecting %d/%d", it->second->pos, top_of_p_table);
		#endif
		if(!*it->second->req)
		{
			send_to_char(ch, "Ошибка: пустой параметр для поиска");//впринципе никогда не должно вылезти, но на всякий случай воткнул проверку
			break;
		}
		if ((it->second->sfor == ICHAR && it->second->unique == player_table[it->second->pos].unique)//Это тот же перс которого мы статим
			|| (player_table[it->second->pos].level >= LVL_IMMORT && !IS_GRGOD(ch))//Иммов могут чекать только 33+
			|| (player_table[it->second->pos].level > GET_LEVEL(ch) && !IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO)))//если левел больше то облом
				continue;
		buf1[0] = '\0';
		buf2[0] = '\0';
		is_online = 0;
		vict = 0;
		d_vict = DescByUID(player_table[it->second->pos].unique);
		if (d_vict)
			is_online = 1;
		if (it->second->sfor != IMAIL && it->second->fullsearch)
		{
			if (d_vict)
				vict = d_vict->character;
			else
			{
				vict = new Player;
				if (load_char(player_table[it->second->pos].name, vict) < 0)
				{
					send_to_char(ch, "Некорректное имя персонажа (%s) inspecting %s: %s.\r\n", player_table[it->second->pos].name, (it->second->sfor==IMAIL?"mail":(it->second->sfor==IIP?"ip":"char")), it->second->req);
					delete vict;
					continue;
				}
			}
			show_pun(vict, buf2);
		}
		if (it->second->sfor == IMAIL || it->second->sfor == ICHAR)
		{
			mail_found = 0;
			if(player_table[it->second->pos].mail)
			 if((it->second->sfor == IMAIL && strstr(player_table[it->second->pos].mail, it->second->req)) || (it->second->sfor == ICHAR && !strcmp(player_table[it->second->pos].mail, it->second->mail)))
				mail_found = 1;
		}
		if (it->second->sfor == IIP || it->second->sfor == ICHAR)
		{
			if(!it->second->fullsearch)
			{
				if(player_table[it->second->pos].last_ip)
				 if((it->second->sfor == IIP && strstr(player_table[it->second->pos].last_ip, it->second->req)) || (it->second->ip_log && !str_cmp(player_table[it->second->pos].last_ip, it->second->ip_log->ip)))
					sprintf(buf1 + strlen(buf1), " IP:%s%-16s%s\r\n", (it->second->sfor == ICHAR? CCBLU(ch, C_SPR) : ""), player_table[it->second->pos].last_ip, (it->second->sfor == ICHAR? CCNRM(ch, C_SPR) : ""));
			}
			else if (vict && LOGON_LIST(vict))
			{
				struct logon_data * cur_log = LOGON_LIST(vict);
				while (cur_log)
				{
					struct logon_data * ch_log = it->second->ip_log;
					if(cur_log->ip)
					 while(ch_log)
					{
						if(!ch_log->ip)
						{
							send_to_char(ch, "Ошибка: пустой ip\r\n");//поиск прерываеться если криво заполнено поле ip для поиска
							break;
						}
						if((it->second->sfor == IIP && strstr(cur_log->ip, ch_log->ip)) || !str_cmp(cur_log->ip, ch_log->ip))
						{
							sprintf(buf1 + strlen(buf1), " IP:%s%-16s%sCount:%5ld Last: %-30s%s",
								(it->second->sfor == ICHAR? CCBLU(ch, C_SPR) : ""), cur_log->ip, (it->second->sfor == ICHAR? CCNRM(ch, C_SPR) : ""), cur_log->count, rustime(localtime(&cur_log->lasttime)),(it->second->sfor == IIP?"\r\n":""));
							if(it->second->sfor == ICHAR)
								sprintf(buf1 + strlen(buf1), "-> Count:%5ld Last : %s\r\n",
									ch_log->count, rustime(localtime(&ch_log->lasttime)));
						}
						ch_log = ch_log->next;
					}
					cur_log = cur_log->next;
				}
			}
		}
		if ((vict) && (!d_vict))
			delete vict;
		if (*buf1 || mail_found)
		{
			mytime = player_table[it->second->pos].last_logon;
			sprintf(buf, "Имя: %s%-12s%s e-mail: %s&S%-30s&s%s Last: %s\r\n",
				(is_online ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)), player_table[it->second->pos].name, CCNRM(ch, C_SPR),
				(mail_found && it->second->sfor!=IMAIL? CCBLU(ch, C_SPR) : ""),
				player_table[it->second->pos].mail, (mail_found? CCNRM(ch, C_SPR) : ""),
				rustime(localtime(&mytime)));
			it->second->out += buf;
			it->second->out += buf2;
			it->second->out += buf1;
			it->second->found++;
		}
	}
	while (it->second->ip_log)
	{
		struct logon_data *log_next;
		log_next = it->second->ip_log->next;
		free(it->second->ip_log->ip);
		delete it->second->ip_log;
		it->second->ip_log = log_next;
	}
	if (it->second->mail)
		free(it->second->mail);
	need_warn = true;
	gettimeofday(&stop, NULL);
	timediff(&result, &stop, &it->second->start);
	sprintf(buf1, "Всего найдено: %d за %ldсек.\r\n", it->second->found, result.tv_sec);
	it->second->out += buf1;
	page_string(ch->desc, it->second->out);
	free(it->second->req);
	inspect_list.erase(it->first);
}

ACMD(do_inspect)//added by WorM Команда для поиска чаров с одинаковым(похожим) mail и/или ip
{
	DESCRIPTOR_DATA *d_vict = 0;
	CHAR_DATA *vict = 0;
	int i = 0;

	if (ch->get_pfilepos() < 0)
		return;

	InspReqListType::iterator it = inspect_list.find(GET_UNIQUE(ch));
	SetAllInspReqListType::iterator it_setall = setall_inspect_list.find(GET_UNIQUE(ch));
	// Навсякий случай разрешаем только одну команду такого типа, либо сетол, либо инспект
	if (it != inspect_list.end() and it_setall != setall_inspect_list.end())
	{
		send_to_char(ch, "Обрабатывается другой запрос, подождите...\r\n", argument);
		return;
	}
	argument = two_arguments(argument, buf, buf2);
	if (!*buf || !*buf2 || !a_isascii(*buf2))
	{
		send_to_char("Usage: inspect { mail | ip | char } <argument> [all|все]\r\n", ch);
		return;
	}
	if(!isname(buf, "mail ip char"))
	{
		send_to_char("Нет уж. Изыщите другую цель для своих исследований.\r\n", ch);
		return;
	}
	if(strlen(buf2)<3)
	{
		send_to_char("Слишком короткий запрос\r\n", ch);
		return;
	}
	if(strlen(buf2)>65)
	{
		send_to_char("Слишком длинный запрос\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "char") && (GetUniqueByName(buf2)<=0))
	{
		send_to_char(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", buf2);
		return;
	}
	InspReqPtr req(new inspect_request);
	req->ip_log = NULL;
	req->mail = NULL;
	req->fullsearch = 0;
	req->req = str_dup(buf2);
	buf2[0] = '\0';

	if(argument && isname(argument, "все all"))
	{
		if(IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			need_warn = false;
			req->fullsearch = 1;
		}
	}
	if (is_abbrev(buf, "mail"))
	{
		req->sfor = IMAIL;
	}
	else if (is_abbrev(buf, "ip"))
	{
		req->sfor = IIP;
		if(req->fullsearch)
		{
			req->ip_log = new(struct logon_data);
			req->ip_log->ip = str_dup(req->req);
			req->ip_log->count = 0;
			req->ip_log->lasttime = 0;
			req->ip_log->next = 0;
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
		sprintf(buf,  "Персонаж: %s%s%s e-mail: %s&S%s&s%s Last: %s%s%s from IP: %s%s%s\r\n", (d_vict ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR)),
				player_table[i].name, CCNRM(ch, C_SPR),	CCWHT(ch, C_SPR), req->mail, CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR), rustime(localtime(&tmp_time)), CCNRM(ch, C_SPR),
				CCWHT(ch, C_SPR), player_table[i].last_ip, CCNRM(ch, C_SPR));
		if (req->fullsearch)
		{
			if (d_vict)
				vict = d_vict->character;
			else
			{
				vict = new Player;
				if (load_char(req->req, vict) < 0)
				{
					send_to_char(ch, "Некорректное имя персонажа (%s) inspecting char.\r\n", req->req);
					delete vict;
					return;
				}
			}
			show_pun(vict, buf2);
			if (vict && LOGON_LIST(vict))
			{
				#ifdef TEST_BUILD
				log("filling logon list");
				#endif
				struct logon_data * cur_log = LOGON_LIST(vict);
				struct logon_data * tmp_log = NULL;
				i = 0;
				while (cur_log)
				{
					#ifdef TEST_BUILD
					log("filling logon list %d", i);
					#endif
					if(i == 0)
					{
						req->ip_log = new(struct logon_data);
						tmp_log = req->ip_log;
					}
					else
					{
						req->ip_log->next = new(struct logon_data);
						tmp_log = req->ip_log->next;
					}
					tmp_log->ip = str_dup(cur_log->ip);
					tmp_log->count = cur_log->count;
					tmp_log->lasttime = cur_log->lasttime;
					tmp_log->next = 0;
					i++;
					cur_log = cur_log->next;
				}
			}
			if (!d_vict && vict)
				delete vict;
		}
		else
		{
			req->ip_log = new(struct logon_data);
			req->ip_log->ip = str_dup(player_table[i].last_ip);
			req->ip_log->count = 0;
			req->ip_log->lasttime = player_table[i].last_logon;
			req->ip_log->next = 0;
		}
	}
	if (req->sfor < ICHAR)
		sprintf(buf,  "%s: %s&S%s&s%s\r\n", (req->sfor==IIP?"IP":"e-mail"),
			CCWHT(ch, C_SPR), req->req, CCNRM(ch, C_SPR));
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
ACMD(do_syslog)
{
	int tp;

	if (subcmd < 0 || subcmd >= NLOG)
		return;

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
		if (((tp = search_block(arg, logtypes, FALSE)) == -1))
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
	sprintf(buf, "Тип вашего лога (%s) сейчас %s.\r\n", logs[subcmd].name, logtypes[tp]);
	send_to_char(buf, ch);
	return;
}



ACMD(do_advance)
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



ACMD(do_restore)
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
		GET_DRUNK_STATE(vict)=GET_COND(vict, DRUNK)=0;
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
			!AFF_FLAGGED(ch, AFF_HIDE) && !AFF_FLAGGED(ch, AFF_INVISIBLE) && !AFF_FLAGGED(ch, AFF_CAMOUFLAGE))
	{
		send_to_char("Ну вот вас и заметили. Стало ли вам легче от этого?\r\n", ch);
		return;
	}

	GET_INVIS_LEV(ch) = 0;
	appear(ch);
	send_to_char("Вы теперь полностью видны.\r\n", ch);
}


void perform_immort_invis(CHAR_DATA * ch, int level)
{
	CHAR_DATA *tch;

	if (IS_NPC(ch))
		return;

	for (tch = world[ch->in_room]->people; tch; tch = tch->next_in_room)
	{
		if (tch == ch)
			continue;
		if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
			act("Вы вздрогнули, когда $n растворил$u на ваших глазах.", FALSE, ch, 0, tch, TO_VICT);
		if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
			act("$n медленно появил$u из пустоты.", FALSE, ch, 0, tch, TO_VICT);
	}

	GET_INVIS_LEV(ch) = level;
	sprintf(buf, "Ваш уровень невидимости - %d.\r\n", level);
	send_to_char(buf, ch);
}


ACMD(do_invis)
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


ACMD(do_gecho)
{
	DESCRIPTOR_DATA *pt;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
		send_to_char("Это, пожалуй, ошибка...\r\n", ch);
	else
	{
		sprintf(buf, "%s\r\n", argument);
		for (pt = descriptor_list; pt; pt = pt->next)
			if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
				send_to_char(buf, pt->character);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
			send_to_char(buf, ch);
	}
}


ACMD(do_poofset)
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



ACMD(do_dc)
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



ACMD(do_wizlock)
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


ACMD(do_date)
{
	char *tmstr;
	time_t mytime;
	int d, h, m, s;

	if (subcmd == SCMD_DATE)
		mytime = time(0);
	else
		mytime = boot_time;

	tmstr = (char *) asctime(localtime(&mytime));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	if (subcmd == SCMD_DATE)
		sprintf(buf, "Текущее время сервера : %s\r\n", tmstr);
	else
	{
		mytime = time(0) - boot_time;
		d = mytime / 86400;
		h = (mytime / 3600) % 24;
		m = (mytime / 60) % 60;
		s = mytime % 60;

		sprintf(buf, "Up since %s: %d day%s, %d:%02d.%02d\r\n", tmstr, d, ((d == 1) ? "" : "s"), h, m, s);
	}

	send_to_char(buf, ch);
}

ACMD(do_last)
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
				GET_IDNUM(chdata), (int) GET_LEVEL(chdata),
				kin_abbrevs[(int) GET_KIN(chdata)],
				class_abbrevs[(int) GET_CLASS(chdata)], GET_NAME(chdata),
				GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown", ctime(&tmp_time));
		send_to_char(buf, ch);
	}
}

ACMD(do_force)
{
	DESCRIPTOR_DATA *i, *next_desc;
	CHAR_DATA *vict, *next_force;
	char to_force[MAX_INPUT_LENGTH + 2];

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n принудил$g вас '%s'.", to_force);

	if (!*arg || !*to_force)
		send_to_char("Кого и что вы хотите принудить сделать?\r\n", ch);
	else if (!IS_GRGOD(ch) || (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg)
							   && str_cmp("здесь", arg)))
	{
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
			send_to_char(NOPERSON, ch);
		else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && !PRF_FLAGGED(ch, PRF_CODERINFO))
			send_to_char("Господи, только не это!\r\n", ch);
		else
		{
			char *pstr;
			send_to_char(OK, ch);
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			while ((pstr = strstr(buf, "%")) != NULL)
				pstr[0] = '*';
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
			imm_log("%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			command_interpreter(vict, to_force);
		}
	}
	else if (!str_cmp("room", arg) || !str_cmp("здесь", arg))
	{
		send_to_char(OK, ch);
		sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

		for (vict = world[ch->in_room]->people; vict; vict = next_force)
		{
			next_force = vict->next_in_room;
			if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
				continue;
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

			if (STATE(i) != CON_PLAYING
					|| !(vict = i->character)
					|| (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)
						&& !PRF_FLAGGED(ch, PRF_CODERINFO)))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	}
}


ACMD(do_sdemigod)
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
		    send_to_char(buf1, d->character);
		}
	    }
	}
    }
}


ACMD(do_wiznet)
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

	if (PRF_FLAGGED(ch, PRF_CODERINFO)) return;

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
				((GET_LEVEL(d->character) >= level)
// ||	// уровень равным или выше level
//				 (GET_LEVEL(d->character) < LVL_IMMORT &&	// игроки с флагом 'gd_demigod' могут видеть теллы в имм канал,
//				  GET_GOD_FLAG(d->character, GF_DEMIGOD) &&	// за исключением случая когда level > LVL_IMMORT
//				  level <= LVL_IMMORT)
				) && (!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&	// игрок с режимом NOWIZ не видит имм канала
				(!PLR_FLAGGED(d->character, PLR_WRITING)) &&	// пишущий не видит имм канала
				(!PLR_FLAGGED(d->character, PLR_MAILING)))	// отправляющий письмо не видит имм канала
		{
			// отправляем сообщение чару
			snprintf(buf2, MAX_STRING_LENGTH, "%s%s%s",
					CCCYN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM));
			d->character->remember_add(buf2, Remember::ALL);
			// не видино своих мессаг если 'режим repeat'
			if (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))
			{
				send_to_char(buf2, d->character);
			}
		}
	}

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
}



ACMD(do_zreset)
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
		for (i = 0; i <= top_of_zone_table; i++)
			reset_zone(i);
		send_to_char("Перезагружаю мир.\r\n", ch);
		sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s reset entire world.", GET_NAME(ch));
		return;
	}
	else if (*arg == '.')
		i = world[ch->in_room]->zone;
	else
	{
		j = atoi(arg);
		for (i = 0; i <= top_of_zone_table; i++)
			if (zone_table[i].number == j)
				break;
	}
	if (i >= 0 && i <= top_of_zone_table)
	{
		reset_zone(i);
		sprintf(buf, "Перегружаю зону %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
		send_to_char(buf, ch);
		sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
		mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		imm_log("%s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
	}
	else
		send_to_char("Нет такой зоны.\r\n", ch);
}


// Функции установки разных наказаний.


// *  General fn for wizcommands of the sort: cmd <player>
ACMD(do_wizutil)
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
			set_punish(ch, vict, SCMD_UNREGISTER, reason, 0);
			break;

		case SCMD_UNREGISTER:
			if (*num) times = atol(num);
			set_punish(ch, vict, SCMD_UNREGISTER, reason, times);
			break;

		case SCMD_UNAFFECT:
			if (vict->affected)
			{
				while (vict->affected)
					affect_remove(vict, vict->affected);
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
	char tmpstr[255];
	sprintf(tmpstr,
			"%3d %-30.30s Level: %2d; Type: %-10.10s; Age: %3d; Reset: %3d (%1d)(%1d)\r\n"
			"    Top: %5d %s%s; ResetIdle: %s; Used: %s; Activity: %.2f; Group: %2d, Mob-level: %2d\r\n",
			zone_table[zone].number, zone_table[zone].name,
			zone_table[zone].level, zone_types[zone_table[zone].type].name,
			zone_table[zone].age, zone_table[zone].lifespan,
			zone_table[zone].reset_mode,
			(zone_table[zone].reset_mode == 3) ? (can_be_reset(zone) ? 1 : 0) : (is_empty(zone) ? 1 : 0),
					zone_table[zone].top,
					zone_table[zone].under_construction ? "T" : " ",
					zone_table[zone].locked ? " L" : " ",
					zone_table[zone].reset_idle ? "Y" : "N",
					zone_table[zone].used ? "Y" : "N",
					(double)zone_table[zone].activity / 1000,
					zone_table[zone].group,
					zone_table[zone].mob_level);
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
					&& world[world[n]->dir_option[dir]->to_room]->zone != zone
					&& world[world[n]->dir_option[dir]->to_room]->number > 0)
				{
					snprintf(tmp, sizeof(tmp),
						"  Номер комнаты:%5d Направление:%6s Выход в комнату:%5d\r\n",
						world[n]->number, Dirs[dir],
						world[world[n]->dir_option[dir]->to_room]->number);
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
					&& world[world[n]->dir_option[dir]->to_room]->zone == zone
					&& world[world[n]->dir_option[dir]->to_room]->number > 0)
				{
					snprintf(tmp, sizeof(tmp),
						"  Номер комнаты:%5d Направление:%6s Вход в комнату:%5d\r\n",
						world[n]->number, Dirs[dir],
						world[world[n]->dir_option[dir]->to_room]->number);
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
			zone_table[mob_index[mob_rnum].zone].name  : "EMPTY" ;

		out += boost::str(boost::format("%3d %31s [%2d][%6d] %31s\r\n")
			% ++cnt
			% (mob_proto[mob_rnum].get_name_str().size() > 31
				? mob_proto[mob_rnum].get_name_str().substr(0, 31)
				: mob_proto[mob_rnum].get_name_str())
			% zone_table[mob_index[mob_rnum].zone].mob_level
			% GET_MOB_VNUM(mob_proto + mob_rnum)
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

ACMD(do_show)
{
	int i, j, k, l, con;	// i, j, k to specifics?

	zone_rnum zrn;
	zone_vnum zvn;
	char self = 0;
	CHAR_DATA *vict;
	OBJ_DATA *obj;
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
			for (zrn = 0; zrn <= top_of_zone_table; zrn++)
				if (zone_table[zrn].number >= zstart &&
						zone_table[zrn].number <= zend)
				{
					print_zone_to_buf(&bf, zrn);
					found = 1;
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
					zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
			if (zrn <= top_of_zone_table)
				print_zone_to_buf(&bf, zrn);
			else
			{
				send_to_char("Нет такой зоны.\r\n", ch);
				return;
			}
		}
		else if (*value && !strcmp(value, "-g"))
		{
			for (zrn = 0; zrn <= top_of_zone_table; zrn++)
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
				for (zrn = 0; zrn <= top_of_zone_table; zrn++)
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
				for (zrn = 0; zrn <= top_of_zone_table; zrn++)
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
			for (zrn = 0; zrn <= top_of_zone_table; zrn++)
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
		sprinttype(GET_SEX(vict), genders, buf + strlen(buf));
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
		sprintf(buf + strlen(buf), rem);
		sprintf(buf + strlen(buf), "Уровень: %s\r\n", (GET_LEVEL(vict) < 25 ? "ниже 25" : "25+"));
		sprintf(buf + strlen(buf), "Титул: %s\r\n", (vict->player_data.title ? vict->player_data.title : "<Нет>"));
		sprintf(buf + strlen(buf), "Описание игрока:\r\n");
		sprintf(buf + strlen(buf), "%s\r\n", (vict->player_data.description ? vict->player_data.description : "<Нет>"));
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
		k = 0;
		con = 0;
		int motion = 0;
		for (vict = character_list; vict; vict = vict->next)
		{
			if (IS_NPC(vict))
				j++;
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
						con++;
				}
			}
		}
		for (obj = object_list; obj; obj = obj->next)
			k++;
		strcpy(buf, "Текущее состояние:\r\n");
		sprintf(buf + strlen(buf), "  Игроков в игре - %5d, соединений - %5d\r\n", i, con);
		sprintf(buf + strlen(buf), "  Всего зарегистрировано игроков - %5d\r\n", top_of_p_table + 1);
		sprintf(buf + strlen(buf), "  Мобов - %5d,  прообразов мобов - %5d\r\n", j, top_of_mobt + 1);
		sprintf(buf + strlen(buf), "  Предметов - %5d, прообразов предметов - %5d\r\n", k, top_of_objt + 1);
		sprintf(buf + strlen(buf), "  Комнат - %5d, зон - %5d\r\n", top_of_world + 1, top_of_zone_table + 1);
		sprintf(buf + strlen(buf), "  Больших буферов - %5d\r\n", buf_largecount);
		sprintf(buf + strlen(buf), "  Переключенных буферов - %5d, переполненных - %5d\r\n", buf_switches, buf_overflows);
		sprintf(buf + strlen(buf), "  Послано байт - %lu\r\n", number_of_bytes_written);
		sprintf(buf + strlen(buf), "  Получено байт - %lu\r\n", number_of_bytes_read);
		sprintf(buf + strlen(buf), "  Максимальный ID - %ld\r\n", max_id);
		sprintf(buf + strlen(buf), "  Активность игроков (cmds/min) - %lu\r\n", (cmd_cnt * 60) / (time(0) - boot_time));
		send_to_char(buf, ch);
		Depot::show_stats(ch);
		Glory::show_stats(ch);
		GloryConst::show_stats(ch);
		Parcel::show_stats(ch);
		send_to_char(ch, "  Сообщений на почте: %zu\r\n", mail::get_msg_count());
		send_to_char(ch, "  Список полей сражающихся: %d\r\n", fighting_list_size());
		send_to_char(ch, "  Передвижения: %d\r\n", motion);
		send_to_char(ch, "  Потрачено кун в магазинах2 за ребут: %d\r\n", ShopExt::get_spent_today());
		mob_stat::show_stats(ch);
		break;
	}
	case 5:
		strcpy(buf, "Пустых выходов\r\n" "--------------\r\n");
		for (i = FIRST_ROOM, k = 0; i <= top_of_world; i++)
			for (j = 0; j < NUM_OF_DIRS; j++)
				if (world[i]->dir_option[j]
						&& world[i]->dir_option[j]->to_room == 0)
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k,
							GET_ROOM_VNUM(i), world[i]->name);
		page_string(ch->desc, buf, TRUE);
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
		for (i = 0, vict = character_list; vict; vict = vict->next)
		{
			if (IS_NPC(vict) || vict->desc != NULL || IN_ROOM(vict) == NOWHERE)
				continue;
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
						(FREEZE_DURATION(d->character) - time(NULL)) / 3600,
						FREEZE_REASON(d->character) ? FREEZE_REASON(d->character)
						: "-");

			if (PLR_FLAGGED(d->character, PLR_MUTE)
					&& MUTE_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет молчать : %ld час [%s].\r\n",
						(MUTE_DURATION(d->character) - time(NULL)) / 3600,
						MUTE_REASON(d->character) ? MUTE_REASON(d->character) : "-");

			if (PLR_FLAGGED(d->character, PLR_DUMB)
					&& DUMB_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет нем : %ld час [%s].\r\n",
						(DUMB_DURATION(d->character) - time(NULL)) / 3600,
						DUMB_REASON(d->character) ? DUMB_REASON(d->character) : "-");

			if (PLR_FLAGGED(d->character, PLR_HELLED)
					&& HELL_DURATION(d->character))
				sprintf(buf + strlen(buf), "Будет в аду : %ld час [%s].\r\n",
						(HELL_DURATION(d->character) - time(NULL)) / 3600,
						HELL_REASON(d->character) ? HELL_REASON(d->character) : "-");

			if (!PLR_FLAGGED(d->character, PLR_REGISTERED)
					&& UNREG_DURATION(d->character))
				sprintf(buf + strlen(buf), "Не сможет заходить с одного IP : %ld час [%s].\r\n",
						(UNREG_DURATION(d->character) - time(NULL)) / 3600,
						UNREG_REASON(d->character) ? UNREG_REASON(d->character) : "-");


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
			for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn
				&& zrn <= top_of_zone_table; zrn++)
			{
				// empty
			}
			if (zrn <= top_of_zone_table)
			{
				std::string out = print_zone_exits(zrn);
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

#define SET_OR_REMOVE(flagset, flags) { \
  	   if (on) SET_BIT(flagset, flags); \
	      else \
       if (off) REMOVE_BIT(flagset, flags);}

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
	{"\n", 0, BOTH, MISC}
};

int perform_set(CHAR_DATA * ch, CHAR_DATA * vict, int mode, char *val_arg)
{
	int i, j, c, on = 0, off = 0, value = 0, return_code = 1, ptnum, times = 0;
	char npad[NUM_PADS][256];
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
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_BRIEF), PRF_BRIEF);
		break;
	case 1:
		SET_OR_REMOVE(PLR_FLAGS(vict, PLR_INVSTART), PLR_INVSTART);
		break;
	case 2:
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_SUMMONABLE), PRF_SUMMONABLE);
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
		GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
		break;
	case 18:
		if (!IS_IMPL(ch) && ch != vict && !PRF_FLAGGED(ch, PRF_CODERINFO))
		{
			send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return (0);
		}
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_NOHASSLE), PRF_NOHASSLE);
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
			RANGE(0, 24);
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
		SET_OR_REMOVE(PLR_FLAGS(vict, PLR_THIEF), PLR_THIEF);
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
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_ROOMFLAGS), PRF_ROOMFLAGS);
		break;
	case 29:
		SET_OR_REMOVE(PLR_FLAGS(vict, PLR_SITEOK), PLR_SITEOK);
		break;
	case 30:
		if (IS_IMPL(vict) || PRF_FLAGGED(vict, PRF_CODERINFO))
		{
			send_to_char("Истинные боги вечны!\r\n", ch);
			return 0;
		}
		SET_OR_REMOVE(PLR_FLAGS(vict, PLR_DELETED), PLR_DELETED);
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
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_COLOR_1), PRF_COLOR_1);
		SET_OR_REMOVE(PRF_FLAGS(vict, PRF_COLOR_2), PRF_COLOR_2);
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
		Password::set_password(vict, std::string(val_arg));
		sprintf(output, "Пароль изменен на '%s'.", val_arg);
		break;
	case 37:
		SET_OR_REMOVE(PLR_FLAGS(vict, PLR_NODELETE), PLR_NODELETE);
		break;
	case 38:
		if ((i = search_block(val_arg, genders, FALSE)) < 0)
		{
			send_to_char
			("Может быть 'мужчина', 'женщина', или 'бесполое'(а вот это я еще не оценил :).\r\n", ch);
			return (0);
		}
		GET_SEX(vict) = i;
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
		GET_OLC_ZONE(vict) = value;
		break;
	case 45:
		// изменение имени !!!

		if ((i =
					sscanf(val_arg, "%s %s %s %s %s %s", npad[0], npad[1], npad[2], npad[3], npad[4], npad[5])) != 6)
		{
			sprintf(buf, "Требуется указать 6 падежей, найдено %d\r\n", i);
			send_to_char(buf, ch);
			return (0);
		}

		for (i = 0; i < NUM_PADS; i++)
		{
			if (strlen(npad[i]) < MIN_NAME_LENGTH || strlen(npad[i]) > MAX_NAME_LENGTH)
			{
				sprintf(buf, "Падеж номер %d некорректен.\r\n", ++i);
				send_to_char(buf, ch);
				return (0);
			}
		}

		if (*npad[0] == '*')  	// Only change pads
		{
			for (i = 1; i < NUM_PADS; i++)
				if (!_parse_name(npad[i], npad[i]))
				{
					if (GET_PAD(vict, i))
						free(GET_PAD(vict, i));
					CREATE(GET_PAD(vict, i), char, strlen(npad[i]) + 1);
					strcpy(GET_PAD(vict, i), npad[i]);
				}
			sprintf(buf, "Произведена замена падежей.\r\n");
			send_to_char(buf, ch);
		}
		else
		{
			/*if (!IS_IMPL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
			{
				send_to_char("Для изменения падежей пользуйтесь форматом 'set имя name *падеж1 падеж2 падеж3 падеж4 падеж5 падеж6'.\r\nРенеймы не разрешаются.\r\n", ch);
				return 0;
			}*/

			if (_parse_name(npad[0], npad[0]) ||
					strlen(npad[0]) < MIN_NAME_LENGTH ||
					strlen(npad[0]) > MAX_NAME_LENGTH ||
					!Valid_Name(npad[0]) || reserved_word(npad[0]) || fill_word(npad[0]))
			{
				send_to_char("Некорректное имя.\r\n", ch);
				return (0);
			}

			if (get_id_by_name(npad[0]) >= 0)
			{
				send_to_char("Это имя совпадает с именем другого персонажа.\r\n"
							 "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
				return (0);
			}
			/* раз уже при создании чара это давным давно отключили, то тут парить мозг смысла нету
						if (cmp_ptable_by_name(npad[0], MIN_NAME_LENGTH + 1) >= 0) {
							send_to_char
							    ("Первые  символы указанного имени совпадают с уже существующим персонажем.\r\n"
							     "Для исключения разных недоразумений следует указать другое имя.\r\n", ch);
							return (0);
						}
			        if ((ptnum = cmp_ptable_by_name(npad[0],MIN_NAME_LENGTH)) > 0
			            player_table[ptnum].unique != GET_UNIQUE(vict)
			           )
			           {send_to_char("Первые 4 символа этого имени совпадают еще у одного персонажа.\r\n"
			                         "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
			            return (0);
			           }
			*/
			// выносим из листа неодобренных имен, если есть
			NewNameRemove(vict);

			ptnum = get_ptable_by_name(GET_NAME(vict));
			if (ptnum < 0)
				return (0);

			if (!IS_SET(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN) && !IS_SET(PLR_FLAGS(vict, PLR_DELETED), PLR_DELETED) && !IS_IMMORTAL(vict))
				TopPlayer::Remove(vict);

			for (i = 0; i < NUM_PADS; i++)
			{
				if (!_parse_name(npad[i], npad[i]))
				{
					if (GET_PAD(vict, i))
						free(GET_PAD(vict, i));
					CREATE(GET_PAD(vict, i), char, strlen(npad[i]) + 1);
					strcpy(GET_PAD(vict, i), npad[i]);
				}
			}
			sprintf(buf, "Name changed from %s to %s", GET_NAME(vict), npad[0]);
			vict->set_name(npad[0]);
			add_karma(vict, buf, GET_NAME(ch));

			if (!IS_SET(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN) && !IS_SET(PLR_FLAGS(vict, PLR_DELETED), PLR_DELETED) && !IS_IMMORTAL(vict))
				TopPlayer::Refresh(vict);

			free(player_table[ptnum].name);
			CREATE(player_table[ptnum].name, char, strlen(npad[0]) + 1);
			for (i = 0, player_table[ptnum].name[i] = '\0'; npad[0][i]; i++)
				player_table[ptnum].name[i] = LOWER(npad[0][i]);
			return_code = 2;
			SET_BIT(PLR_FLAGS(vict, PLR_CRASH), PLR_CRASH);
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
		if (!set_punish(ch, vict, SCMD_UNREGISTER, reason, times)) return (0);
		break;

	case 57:      // Установка флага палач
		reason = one_argument(val_arg, num);
		skip_spaces(&reason);
		sprintf(buf, "executor %s by %s", (on?"on":"off"), GET_NAME(ch));
		add_karma(vict, buf, reason);
		if (on)
		{
			SET_BIT(PRF_FLAGS(vict, PRF_EXECUTOR), PRF_EXECUTOR);
		}
		else if (off)
		{
			REMOVE_BIT(PRF_FLAGS(vict, PRF_EXECUTOR), PRF_EXECUTOR);
		}
		break;

	case 58: // Снятие или постановка флага !ДУШЕГУБ! только для имплементоров
        SET_OR_REMOVE(PLR_FLAGS(vict, PLR_KILLER), PLR_KILLER);
        break;

	default:
		send_to_char("Не могу установить это!\r\n", ch);
		return (0);
	}

	strcat(output, "\r\n");
	send_to_char(CAP(output), ch);
	return (return_code);
}



ACMD(do_set)
{
	CHAR_DATA *vict = NULL, *cbuf = NULL;
	char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH], OName[MAX_INPUT_LENGTH];
	int mode, len, player_i = 0, retval;
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
				if (GET_LEVEL(ch) <= GET_LEVEL(vict) && !PRF_FLAGGED(ch, PRF_CODERINFO))
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

		cbuf = new Player; // TODO: переделать на стек

		if ((player_i = load_char(name, cbuf)) > -1)
		{
			// Запрет на злоупотребление командой SET на бессмертных
			if (!GET_GOD_FLAG(ch, GF_DEMIGOD))
			{
				if (GET_LEVEL(ch) <= GET_LEVEL(cbuf) && !PRF_FLAGGED(ch, PRF_CODERINFO))
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					delete cbuf;
					return;
				}
			}
			else
			{
				if (GET_LEVEL(cbuf) >= LVL_IMMORT)
				{
					send_to_char("Вы не можете сделать этого.\r\n", ch);
					delete cbuf;
					return;
				}
			}
			vict = cbuf;
		}
		else
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			delete cbuf;
			return;
		}
	}

	// find the command in the list
	len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
		if (!strncmp(field, set_fields[mode].cmd, len))
			break;

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

	// free the memory if we allocated it earlier
	if (is_file)
		delete cbuf;

	log("(GC) %s try to set: %s", GET_NAME(ch), argument);
	imm_log("%s try to set: %s", GET_NAME(ch), argument);

}

SPECIAL(shop_ext);
SPECIAL(receptionist);
SPECIAL(postmaster);
SPECIAL(bank);
SPECIAL(exchange);
SPECIAL(horse_keeper);
SPECIAL(guild_mono);
SPECIAL(guild_poly);

namespace Mlist
{

std::string print_race(CHAR_DATA *mob)
{
	std::string out;
	if (GET_RACE(mob) < NPC_RACE_LAST)
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

	if (mob_proto[GET_MOB_RNUM(mob)].proto_script)
	{
		bool first = true;
		for (trig_proto_list *trg_proto = mob_proto[GET_MOB_RNUM(mob)].proto_script;
			trg_proto; trg_proto = trg_proto->next)
		{
			const int trg_rnum = real_trigger(trg_proto->vnum);
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
					out += GET_TRIG_NAME(trig_index[trg_rnum]->proto)
						? GET_TRIG_NAME(trig_index[trg_rnum]->proto)
						: "null";
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
		SPECIAL(*func) = mob_index[GET_MOB_RNUM(mob)].func;
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
			out << boost::format("%5d. %45s [%6d] [%2d]%s\r\n")
				% ++cnt
				% (mob_proto[i].get_name_str().size() > 45
					? mob_proto[i].get_name_str().substr(0, 45)
					: mob_proto[i].get_name_str())
				% mob_index[i].vnum
				% mob_proto[i].get_level()
				% print_flag(ch, mob_proto + i, options);
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

ACMD(do_liblist)
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
		default:
			sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
			mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
			break;
		}
		return;
	}
	first = atoi(buf);
	if (*buf2 && isdigit(buf2[0]))
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

	if (first >= last)
	{
		send_to_char("Второе значение должно быть больше первого.\n\r", ch);
		return;
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
				snprintf(buf_, sizeof(buf_), "%5d. [%5d] (%3d) %s\r\n",
					++found, world[nr]->number, world[nr]->zone, world[nr]->name);
				out += buf_;
			}
		}
		break;
	case SCMD_OLIST:
		snprintf(buf_, sizeof(buf_),
			"Список объектов Vnum %d до %d\r\n", first, last);
		out += buf_;
		for (nr = 0; nr <= top_of_objt; nr++)
		{
			if (obj_index[nr].vnum >= first && obj_index[nr].vnum <= last)
			{
				snprintf(buf_, sizeof(buf_), "%5d. %s [%5d] [ilvl=%d]", ++found,
					colored_name(obj_proto[nr]->short_description, 45),
					obj_index[nr].vnum, obj_proto[nr]->get_ilevel());
				out += buf_;
				if (GET_LEVEL(ch) >= LVL_GRGOD || PRF_FLAGGED(ch, PRF_CODERINFO))
				{
					snprintf(buf_, sizeof(buf_), " Игра:%d Пост:%d\r\n",
						obj_index[nr].number, obj_index[nr].stored);
					out += buf_;
				}
				else
				{
					out += "\r\n";
				}
			}
		}
		break;
	case SCMD_MLIST:
	{
		std::string option;
		if (*buf2 && !isdigit(buf2[0]))
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
		for (nr = 0; nr <= top_of_zone_table && (zone_table[nr].number <= last); nr++)
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

ACMD(do_forcetime)
{

	extern void heartbeat(const int missed_pulses);	// from comm.c
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
		else if (m == 's' || isdigit(m))	// seconds
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
		t = (SECS_PER_MUD_HOUR);

	for (m = 0; m < t * PASSES_PER_SEC; m++)
		heartbeat(t * PASSES_PER_SEC - m);

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
	for(std::map<int, SpellCountType>::iterator it=usage.begin();it != usage.end(); ++it)
		it->second.clear();
	usage.clear();
	start = time(0);
}

std::string statToPrint()
{
	std::stringstream out;
	time_t now=time(0);
	char * end_time = str_dup(rustime(localtime(&now)));
	out << rustime(localtime(&SpellUsage::start)) << " - " << end_time << "\n";
	for (std::map<int, SpellCountType>::iterator it = SpellUsage::usage.begin(); it != SpellUsage::usage.end(); ++it)
	{
		out << std::setw(35)<<pc_class_types[it->first] << "\n";
		for (SpellCountType::iterator itt = it->second.begin(); itt !=it->second.end(); ++itt)
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
	if (charClass > NUM_CLASSES || spellNum > MAX_SPELLS)
		return;
	usage[charClass][spellNum]++;
}


ACMD(do_spellstat)
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

ACMD(do_sanitize)
{
	send_to_char("Запущена процедура сбора мусора после праздника...\r\n", ch);
	Celebrates::sanitize();
}

//-Polud

// This is test command for different testings
ACMD(do_godtest)
{
    int skl;
    std::ostringstream buffer;

    skip_spaces(&argument);

	if (!*argument)
	{
        send_to_char("Чувак, укажи ИД проверяемого скилла.\r\n", ch);
		return;
	}
    skl = Skill::GetNumByID(string(argument));
    if (skl ==  SKILL_UNDEFINED)
	{
        send_to_char("Извини, братан, не нашел. :(\r\n", ch);
		return;
	} else {
        buffer << " Найден скилл " << skill_info[skl].name << " под номером " << skl << "\r\n";
	}

	send_to_char(buffer.str(), ch);
}

namespace
{

struct filter_type
{
	filter_type() : type(-1), wear(-1), wear_message(-1), material(-1) {};

	// тип
	int type;
	// куда одевается
	int wear;
	// для названия куда одеть
	int wear_message;
	// материал
	int material;
	// аффекты weap
	vector<int> affect;
	// аффекты apply
	vector<int> affect2;
	// экстрафлаг
	vector<int> affect3;
};

} // namespace

ACMD(do_print_armor)
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
				filter.material = MAT_BULAT;
			else if (is_abbrev(tmpbuf, "бронза"))
				filter.material = MAT_BRONZE;
			else if (is_abbrev(tmpbuf, "железо"))
				filter.material = MAT_IRON;
			else if (is_abbrev(tmpbuf, "сталь"))
				filter.material = MAT_STEEL;
			else if (is_abbrev(tmpbuf, "кованая.сталь"))
				filter.material = MAT_SWORDSSTEEL;
			else if (is_abbrev(tmpbuf, "драг.металл"))
				filter.material = MAT_COLOR;
			else if (is_abbrev(tmpbuf, "кристалл"))
				filter.material = MAT_CRYSTALL;
			else if (is_abbrev(tmpbuf, "дерево"))
				filter.material = MAT_WOOD;
			else if (is_abbrev(tmpbuf, "прочное.дерево"))
				filter.material = MAT_SUPERWOOD;
			else if (is_abbrev(tmpbuf, "керамика"))
				filter.material = MAT_FARFOR;
			else if (is_abbrev(tmpbuf, "стекло"))
				filter.material = MAT_GLASS;
			else if (is_abbrev(tmpbuf, "камень"))
				filter.material = MAT_ROCK;
			else if (is_abbrev(tmpbuf, "кость"))
				filter.material = MAT_BONE;
			else if (is_abbrev(tmpbuf, "ткань"))
				filter.material = MAT_MATERIA;
			else if (is_abbrev(tmpbuf, "кожа"))
				filter.material = MAT_SKIN;
			else if (is_abbrev(tmpbuf, "органика"))
				filter.material = MAT_ORGANIC;
			else if (is_abbrev(tmpbuf, "береста"))
				filter.material = MAT_PAPER;
			else if (is_abbrev(tmpbuf, "драг.камень"))
				filter.material = MAT_DIAMOND;
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
				filter.type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "легкие") || is_abbrev(tmpbuf, "легкая"))
				filter.type = ITEM_ARMOR_LIGHT;
			else if (is_abbrev(tmpbuf, "средние") || is_abbrev(tmpbuf, "средняя"))
				filter.type = ITEM_ARMOR_MEDIAN;
			else if (is_abbrev(tmpbuf, "тяжелые") || is_abbrev(tmpbuf, "тяжелая"))
				filter.type = ITEM_ARMOR_HEAVY;
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
				filter.wear = ITEM_WEAR_BODY;
				filter.wear_message = 3;
			}
			else if (is_abbrev(tmpbuf, "голова"))
			{
				filter.wear = ITEM_WEAR_HEAD;
				filter.wear_message = 4;
			}
			else if (is_abbrev(tmpbuf, "ноги"))
			{
				filter.wear = ITEM_WEAR_LEGS;
				filter.wear_message = 5;
			}
			else if (is_abbrev(tmpbuf, "ступни"))
			{
				filter.wear = ITEM_WEAR_FEET;
				filter.wear_message = 6;
			}
			else if (is_abbrev(tmpbuf, "кисти"))
			{
				filter.wear = ITEM_WEAR_HANDS;
				filter.wear_message = 7;
			}
			else if (is_abbrev(tmpbuf, "руки"))
			{
				filter.wear = ITEM_WEAR_ARMS;
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
			unsigned int len = strlen(tmpbuf);
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
				sprintf(buf,"Неверный аффект предмета: '%s'.\r\n", tmpbuf);
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
	if (filter.wear >= 0)
	{
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (!filter.affect.empty())
	{
		for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
		{
			buffer += weapon_affects[*it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty())
	{
		for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end(); ++it)
		{
			buffer += apply_types[*it];
			buffer += " ";
		}
	}
	if (!filter.affect3.empty())
	{
		for (vector<int>::const_iterator it = filter.affect3.begin(); it != filter.affect3.end(); ++it)
		{
			buffer += extra_bits[*it];
			buffer += " ";
		}
	}
	buffer += "\r\nСредний уровень мобов в зоне | внум предмета  | материал | имя предмета + аффекты если есть\r\n";
	send_to_char(buffer, ch);

	std::multimap<int /* zone lvl */, int /* obj rnum */> tmp_list;
	for (std::vector <OBJ_DATA *>::iterator i = obj_proto.begin(), iend = obj_proto.end(); i != iend; ++i)
	{
		// материал
		if (filter.material >= 0 && filter.material != GET_OBJ_MATER(*i))
		{
			continue;
		}
		// тип
		if (filter.type >= 0 && filter.type != GET_OBJ_TYPE(*i))
		{
			continue;
		}
		// куда можно одеть
		if (filter.wear >= 0 && !CAN_WEAR(*i, filter.wear))
		{
			continue;
		}
		// аффекты
		bool find = true;
		if (!filter.affect.empty())
		{
			for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
			{
				if (!CompareBits((*i)->obj_flags.affects, weapon_affects, *it))
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
			for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it)
			{
				find = false;
				for (int k = 0; k < MAX_OBJ_AFFECT; ++k)
				{
					if ((*i)->affected[k].location == *it)
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
			for (vector<int>::const_iterator it = filter.affect3.begin(); it != filter.affect3.end() && find; ++it)
			{
				//find = true;
				if (!CompareBits((*i)->obj_flags.extra_flags, extra_bits, *it))
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
			int vnum = GET_OBJ_VNUM(*i)/100;
			for (int nr = 0; nr <= top_of_zone_table; nr++)
			{
				if (vnum == zone_table[nr].number)
				{
					tmp_list.insert(std::make_pair(zone_table[nr].mob_level, GET_OBJ_RNUM(*i)));
				}
			}
		}
	}
	std::ostringstream out;
	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i)
	{
		const OBJ_DATA *obj = obj_proto[i->second];
		out << "   "
			<< std::setw(2) << i->first << " | "
			<< std::setw(7) << GET_OBJ_VNUM(obj) << " | "
			<< std::setw(14) << material_name[GET_OBJ_MATER(obj)] << " | "
			<< GET_OBJ_PNAME(obj, 0) << "\r\n";

		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			int drndice = obj->affected[i].location;
			int drsdice = obj->affected[i].modifier;
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
			switch (negative)
			{
			case false:
				if (obj->affected[i].modifier < 0)
				{
					negative = true;
				}
				break;
			case true:
				if (obj->affected[i].modifier < 0)
				{
					negative = false;
				}
				break;
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
		send_to_char(ch, "Всего найдено предметов: %d\r\n\r\n", tmp_list.size());
		page_string(ch->desc, out.str());
	}
	else
	{
		send_to_char("Ничего не найдено.\r\n", ch);
	}
}
